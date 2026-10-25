# XN297 Whitening and CRC: Technical Specification and Implementation

**Scope:** XN297 packet whitening and 16-bit CRC as implemented and reconstructed in the DIY Multiprotocol TX Module's XN297 emulation, including standard and enhanced packet formats, historical CRC correction tables, and reference test vectors.

## 1. Conventions

- Bits within each transmitted byte are processed **most significant bit first**.
- `^` or `⊕` denotes bitwise XOR; `~` denotes bitwise complement.
- `R8(x)` denotes reversal of the eight bits in byte `x`.
- The address length `A` is 3, 4, or 5 bytes; payload length `P` is in bytes.
- `M` denotes the **unwhitened sequence of bits in transmission order** that is covered by the CRC.
- `S[i]` denotes bit `i` of the whitening sequence, starting at the first transmitted address bit (`i = 0`).
- `S[a:b]` denotes the bits at positions `a` through `b-1`, interpreted MSB first when converted to an integer.
- `CRC(I, M)` denotes the uncomplemented CRC-16 register after processing bitstream `M` from initial register value `I`.

The synchronization/preamble sequence precedes the address. It is separate from `M` and from the whitening sequence indexed here. For the XN297 emulation, the 28-bit synchronization pattern is `0xC710F55`; its representation in the underlying radio interface depends on the transceiver used.

## 2. Packet-field ordering

### 2.1 Standard packets

The CRC-covered, unwhitened bitstream is:

```text
address bytes in reverse API order | payload bytes with bits reversed
```

For API address `01 02 03 04 05` and payload `01 02 03 04 05`, the CRC input bytes are:

```text
05 04 03 02 01 80 40 C0 20 A0
```

The address bytes are reversed in **byte order**. The payload bytes are individually reversed in **bit order**. The address bytes are not individually bit-reversed in this representation.

With whitening enabled, the address and payload bits are XORed with consecutive whitening bits. The CRC is then XORed with the **next 16 whitening bits**. Whitening is continuous across the address, packet-control field when present, payload, and CRC; it is not restarted at field boundaries.

The standard CRC input length is:

```text
n = 8 * (A + P)
```

### 2.2 Enhanced packets

Enhanced packets include a packet-control region between the address and payload. The implementation serializes this region as 10 bits:

```text
0 | payload_length[5:0] | PID[1:0] | NO_ACK
```

The payload then follows as a contiguous stream of bit-reversed payload bytes, without byte alignment after the 10-bit packet-control region.

The enhanced CRC input length is:

```text
n = 8 * (A + P) + 10
```

Consequently, the CRC begins **two bits into a byte**. Its 16 bits span parts of three transmitted bytes. The final packed byte has six unused low-order bits, set to zero by the implementation.

## 3. Whitening generator

### 3.1 Parameters

| Property | Value |
|---|---|
| Generator | 7-bit linear-feedback shift register (LFSR) |
| Connection polynomial | `x^7 + x^3 + 1` |
| Initial register value | `0x71` |
| Output bit | Register bit 6 (MSB) |
| Feedback bit | Register bit 6 XOR register bit 2 |
| Register shift | Left by one; feedback enters bit 0 |
| Output-byte assembly | MSB first, eight consecutive output bits |

For register state `r`, one whitening bit and the next state are:

```text
output = (r >> 6) & 1
r_next = ((r << 1) | (((r >> 6) ^ (r >> 2)) & 1)) & 0x7F
```

The initial seven output bits are `1110001`, corresponding to the seed `0x71`.

### 3.2 Bit-level reference implementation

```c
#include <stdint.h>

static uint8_t xn297_whitening_bit(uint8_t *lfsr)
{
    uint8_t output = (*lfsr >> 6) & 1u;
    uint8_t feedback = ((*lfsr >> 6) ^ (*lfsr >> 2)) & 1u;
    *lfsr = (uint8_t)(((*lfsr << 1) | feedback) & 0x7Fu);
    return output;
}

static uint8_t xn297_whitening_byte(uint8_t *lfsr)
{
    uint8_t value = 0;
    for (unsigned i = 0; i < 8; ++i)
        value = (uint8_t)((value << 1) | xn297_whitening_bit(lfsr));
    return value;
}
```

Initialize `uint8_t lfsr = 0x71;` at the start of the packet's address field.

### 3.3 Whitening reference bytes

The first 39 output bytes are:

```text
E3 B1 4B EA 85 BC E5 66
0D AE 8C 88 12 69 EE 1F
C7 62 97 D5 0B 79 CA CC
1B 5D 19 10 24 D3 DC 3F
8E C5 2F AA 16 F3 95
```

These reproduce the historical `xn297_scramble[]` lookup table. The LFSR generates additional whitening bits beyond the end of that table, including the bits needed for enhanced-packet CRCs.

### 3.4 Eight-bit-at-a-time generator

Let `s[i]` denote the individual whitening bits. The seven-bit recurrence is:

```text
s[i] = s[i-7] ^ s[i-3]       for i >= 7
```

Over GF(2), raising the connection polynomial to the fourth power gives:

```text
(x^7 + x^3 + 1)^4 = x^28 + x^12 + 1
```

Therefore the same sequence also satisfies:

```text
s[i] = s[i-28] ^ s[i-12]     for i >= 28
```

This permits an eight-bit-at-a-time generator using a 32-bit word containing the **next 28 whitening bits in bits 31..4**. Its initial value is the first 28 stream bits followed by four zero bits:

```text
0xE3B14BE0
```

```c
#include <stdint.h>

static uint8_t xn297_next_byte_optimized(uint32_t *state)
{
    uint8_t output = (uint8_t)(*state >> 24);
    *state = (*state << 8) |
             ((uint32_t)(uint8_t)(output ^ (*state >> 8)) << 4);
    return output;
}
```

Initialize `uint32_t state = 0xE3B14BE0u;` when whitening is enabled, or `state = 0;` when whitening is disabled. The zero state makes the same XOR-based packing operations emit unmodified data.

At an aligned byte boundary, after all CRC-input bytes have been processed, the next 16 whitening bits are:

```c
uint16_t crc_whitening = (uint16_t)(state >> 16);
```

For an enhanced packet with two additional CRC-input bits after the last full byte, the CRC whitening mask is:

```c
uint16_t crc_whitening = (uint16_t)(state >> 14);
```

The second expression skips the two whitening bits consumed by the final partial data byte.

## 4. CRC algorithm

### 4.1 CRC parameters

The packet-level CRC model for standard packets and scrambled enhanced packets is:

| Property | Value |
|---|---|
| Width | 16 bits |
| Polynomial | `0x1021` (`x^16 + x^12 + x^5 + 1`) |
| Initial register | `0xFFFF` |
| Input bit order | MSB first |
| Reflected input | No |
| Reflected output | No |
| Final XOR | `0xFFFF` |
| Transmitted CRC bit order | MSB first |
| CRC whitening | XOR with the next 16 whitening bits when enabled |

These parameters correspond to the CRC-16/GENIBUS parameterization of the CCITT polynomial. They are distinct from CRC-16/CCITT-FALSE, which uses a final XOR of zero.

For one input bit `b` and current register `c`:

```text
feedback = ((c >> 15) ^ b) & 1
c = (c << 1) & 0xFFFF
if feedback: c ^= 0x1021
```

A reference function is:

```c
static uint16_t crc1021_bit(uint16_t crc, uint8_t bit)
{
    uint8_t feedback = (uint8_t)(((crc >> 15) ^ bit) & 1u);
    crc = (uint16_t)(crc << 1);
    if (feedback)
        crc ^= 0x1021u;
    return crc;
}
```

### 4.2 CRC over unwhitened data

For a CRC input bitstream `M` of length `n`, define:

```text
C_raw   = CRC(0xFFFF, M)
C_final = C_raw ^ 0xFFFF
```

With whitening enabled, the transmitted CRC is:

```text
C_tx = C_final ^ S[n:n+16]
```

With whitening disabled, the standard-packet transmitted CRC is simply `C_final`.

The whitening applied to the CRC uses whitening bits at **bit offsets `n` through `n+15`**, not the first 16 bits of the whitening sequence. In standard packets those bits are byte-aligned; in enhanced packets they begin two bits into a byte.

### 4.3 Processing order

For standard packets, the encoder performs the following operations in order:

1. Initialize the CRC register to `0xFFFF` and the whitening generator at the beginning of the address.
2. Feed each unwhitened address byte into the CRC in transmitted byte order; XOR the same byte with its whitening byte for transmission.
3. Feed each bit-reversed payload byte into the CRC; XOR it with the next whitening byte for transmission.
4. Complement the CRC register and XOR it with the next 16 whitening bits.
5. Append the resulting CRC most-significant byte first.

The corresponding decoder XORs each received data byte with the same whitening stream **before** feeding it into the CRC. It then compares the received CRC with the complemented calculated CRC XORed with the 16 CRC whitening bits.

For enhanced packets, the same process applies to the extra packet-control bits and the non-byte-aligned end of the payload.

## 5. Historical `0xB5D2` CRC model and XOROUT correction

Earlier XN297 emulation computed the CRC directly over the **already whitened, on-air data bits**, using initial register `0xB5D2`. It then XORed the CRC result with a correction value that depended on the packet length and mode.

Let:

```text
I_old = 0xB5D2
I_new = 0xFFFF
D     = I_old ^ I_new = 0x4A2D
```

With whitening enabled, the on-air data bitstream is `M ^ S[0:n]`. The old intermediate value is:

```text
C_old = CRC(0xB5D2, M ^ S[0:n])
```

The transmitted CRC defined in section 4 is:

```text
C_tx = CRC(0xFFFF, M) ^ 0xFFFF ^ S[n:n+16]
```

Because an uncomplemented CRC register transition is linear over GF(2), their difference is independent of the contents of `M`:

```text
XOROUT(n, scrambled)
    = CRC(0x4A2D, S[0:n]) ^ 0xFFFF ^ S[n:n+16]
```

The old calculation is therefore equivalent to:

```text
C_tx = C_old ^ XOROUT(n, scrambled)
```

When whitening is disabled, substitute zero for every whitening bit:

```text
XOROUT(n, unscrambled)
    = CRC(0x4A2D, n zero bits) ^ 0xFFFF
```

This explains why a correction indexed by packet length can compensate for both the different CRC initialization and the use of whitened rather than unwhitened CRC input.

### 5.1 Reference correction function

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint16_t xn297_crc_xorout(bool scrambled, size_t nbits)
{
    uint16_t correction = 0xFFFFu ^ 0xB5D2u;
    uint8_t lfsr = 0x71u;

    for (size_t i = 0; i < nbits; ++i) {
        uint8_t mask_bit = xn297_whitening_bit(&lfsr);
        correction = crc1021_bit(correction,
                                 scrambled ? mask_bit : 0u);
    }

    uint16_t crc_mask = 0;
    for (unsigned i = 0; i < 16; ++i) {
        uint8_t mask_bit = xn297_whitening_bit(&lfsr);
        crc_mask = (uint16_t)((crc_mask << 1) |
                              (scrambled ? mask_bit : 0u));
    }

    return (uint16_t)(correction ^ 0xFFFFu ^ crc_mask);
}
```

This function uses `xn297_whitening_bit()` and `crc1021_bit()` from sections 3 and 4.

### 5.2 Old lookup-table indexing

Both standard and enhanced correction tables used:

```text
index = A - 3 + P
```

The bit count supplied to the computed replacement is:

| Packet type | CRC-covered bits |
|---|---|
| Standard | `8 * (A + P)` |
| Enhanced | `8 * (A + P) + 10` |

The original source contained four correction tables: standard scrambled, standard unscrambled, enhanced scrambled, and enhanced unscrambled. Entry zero in each table was a placeholder.

The computed formula matches **135 of the 136 non-placeholder historical entries**. The sole discrepancy is the standard scrambled entry for `A = 3`, `P = 1`:

```text
Historical table: 0x3448
Computed value:   0xE348
```

The new correction implementation uses `0xE348`. The remaining historical entries follow the same formula.

### 5.3 Enhanced unscrambled compatibility behavior

The multiprotocol firmware's enhanced-packet **transmit** path retains a distinct legacy convention when scrambling is disabled:

```text
CRC init = 0xB5D2
Final XOR = 0x0000
CRC whitening = disabled
```

That path therefore transmits `CRC(0xB5D2, M)` without applying the general `0xFFFF`-initialized, `0xFFFF`-complemented model. This is an implementation-specific distinction and must be preserved when reproducing that firmware behavior.

The XN297Dump analyzer also contains a computed correction path for enhanced unscrambled packets using the general `XOROUT(n, unscrambled)` formula. The analyzer and transmit-path conventions should not be conflated.

## 6. Enhanced-packet CRC alignment

An enhanced packet's unwhitened CRC input consists of:

```text
reversed-order address bytes
+ 10-bit packet-control region
+ bit-reversed payload bytes
```

With `A` address bytes and `P` payload bytes:

```text
n = 8 * (A + P) + 10
n mod 8 = 2
```

Thus the last two data bits occupy the most significant two bits of a byte. The first six bits of the 16-bit CRC occupy the remaining six bits of that byte; the next eight CRC bits occupy another byte; and the last two CRC bits occupy the two most significant bits of a third byte.

If `C` is the 16-bit transmitted CRC and `tail` is the encoded final two-bit data field already stored in bits 7..6:

```c
out0 = (uint8_t)(tail | (C >> 10));
out1 = (uint8_t)(C >> 2);
out2 = (uint8_t)(C << 6);
```

The resulting CRC bit layout is:

```text
[data:2][CRC:6]  [CRC:8]  [CRC:2][zero:6]
```

The CRC-input calculation covers the final **two data bits** but does not feed CRC bits or the six trailing padding bits back into the CRC register.

## 7. Reference test vectors

All hexadecimal bytes below use the transmission-order representation defined in section 2, excluding the radio-specific synchronization/preamble sequence.

### 7.1 Whitening sequence

```text
LFSR seed:        71
First eight bytes:E3 B1 4B EA 85 BC E5 66
28-bit seed word: E3B14BE0
```

Both whitening implementations in section 3 produce the same stream.

### 7.2 Standard scrambled packet

```text
Address (API order):        01 02 03 04 05
Payload (API order):        01 02 03 04 05
CRC input, unwhitened:     05 04 03 02 01 80 40 C0 20 A0
Whitening, first 10 bytes: E3 B1 4B EA 85 BC E5 66 0D AE
Transmitted data:          E6 B5 48 E8 84 3C A5 A6 2D 0E
CRC input bit length:      80
CRC(FFFF, input):          8B27
CRC after final XOR:       74D8
CRC whitening mask:        8C88
Transmitted CRC:           F850
```

Cross-check against the historical CRC procedure:

```text
CRC(B5D2, transmitted data): E0B6
XOROUT(80, scrambled):       18E6
E0B6 ^ 18E6:                 F850
```

Complete data-and-CRC sequence:

```text
E6 B5 48 E8 84 3C A5 A6 2D 0E F8 50
```

### 7.3 Enhanced scrambled packet

```text
Address (API order):    01 02 03 04 05
Payload (API order):    01 02
PID:                    0
NO_ACK:                 0
Packet-control bits:    0 | 000010 | 00 | 0
CRC input bit length:   66
CRC(FFFF, input):       125A
CRC after final XOR:    EDA5
CRC whitening mask:     36BA
Transmitted CRC:        DB1F
XOROUT(66, scrambled):  CA52
```

Complete encoded bytes, including the final six zero padding bits:

```text
E6 B5 48 E8 84 B8 C5 76 36 C7 C0
```

The enhanced CRC begins two bits into the ninth byte of this encoded sequence.

### 7.4 Short-length historical-table correction

For a three-byte address and one-byte payload in standard scrambled mode:

```text
A = 3
P = 1
n = 32 bits
XOROUT(32, scrambled) = E348
```

## 8. Implementation integration

The optimized firmware implementation uses a shared 32-bit whitening state to perform whitening, unwhitening, and CRC-mask extraction without the historical `xn297_scramble[]` table. The standard packet encoder/decoder computes the CRC over **unwhitened** bytes and applies the CRC whitening mask directly. The enhanced implementation uses the same model with the additional two-bit alignment.

`XN297Dump` continues to calculate CRC values over received encoded bytes and uses the computed historical correction function to evaluate candidate lengths and scrambling modes. Its local whitening-byte buffer is generated at runtime rather than stored as a global constant table.

The original XOROUT tables are therefore not necessary for the described emulation. The calculated correction depends only on scrambling mode and the number of CRC-covered bits; it does not depend on packet contents.

## 9. Sources and implementation references

1. **DIY Multiprotocol TX Module, PR #1199:** *HoTT, XN297 emu: compute the hop and CRC xorout tables* — derivation of the CRC correction function and comparison with historical tables. <https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/pull/1199>
2. **DIY Multiprotocol TX Module, PR #1201:** *Compute xn297 whitening instead of using an hardcoded array* — 28-bit recurrence, optimized byte generator, direct CRC handling, and enhanced-packet implementation. <https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/pull/1201>
3. **XN297 emulation source at PR #1201 revision `0cfaad0d`:** <https://github.com/goebish/DIY-Multiprotocol-TX-Module/blob/0cfaad0d893ef9e111febbf9b4c2c4019201d83b/Multiprotocol/XN297_EMU.ino>
4. **XN297 interface definitions at the same revision:** <https://github.com/goebish/DIY-Multiprotocol-TX-Module/blob/0cfaad0d893ef9e111febbf9b4c2c4019201d83b/Multiprotocol/iface_xn297.h>
5. **Historical XN297 reverse-engineering materials and HackRF recordings:** <https://github.com/victzh/xn297_reveng>

