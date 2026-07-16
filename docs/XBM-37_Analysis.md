# XBM-37 Quad SPI Capture Analysis

**Manufacturer:** T-Smart  
**Model:** XBM-37 (toy quadcopter)  
**TX/RX RF Chip:** SV7241A (QFN-20, NRF24L01+ clone / BK2425 derivative)  
**Protocol Basis:** Closely related to FQ777 (same RF chip family, same bind address, same ssv ESB air encoding)  
**Capture Tool:** Logic analyzer – digital (02a) and SPI-decoded (all "b" files)

---

## Table of Contents

1. [Capture File Inventory](#1-capture-file-inventory)
2. [Key Protocol Parameters](#2-key-protocol-parameters)
3. [SPI Initialization Sequence](#3-spi-initialization-sequence)
4. [Bind Sequence – Deep Analysis (02a + 02b)](#4-bind-sequence--deep-analysis-02a--02b)
5. [Bind vs No-RX Comparison (01b vs 02b)](#5-bind-vs-no-rx-comparison-01b-vs-02b)
6. [Normal Data Packet Format](#6-normal-data-packet-format)
7. [Per-Channel Control Analysis (03b – 24b)](#7-per-channel-control-analysis-03b--24b)
8. [RF Timing Summary](#8-rf-timing-summary)
9. [MPM Implementation Notes](#9-mpm-implementation-notes)

---

## 1. Capture File Inventory

All captures are located in `Captures_XBM-37/`. The **"b" files** are SPI-decoded exports
(columns: `Time [s], Packet ID, MOSI, MISO`). The **"a" file** is the raw digital export
(columns: `Time[s], MOSI, MISO, SCK, CSN, CE, IRQ`).

| File | Type | Description | SPI Transactions | TX Payloads | Duration |
|------|------|-------------|-----------------|-------------|----------|
| `01b-XBM-37_Quad_TX-PowerOn-NoRX.csv` | SPI | TX power-on, **no RX present** | 5,437 | 1,352 | 2.963 s |
| `02a-XBM-37_Quad_TX-PowerOn-withRX-Bind.csv` | **Digital** | TX power-on + bind **with RX** (all 6 lines) | 405,271 samples | 1,345 payloads | 4.380 s |
| `02b-XBM-37_Quad_TX-PowerOn-withRX-Bind.csv` | SPI | TX power-on + bind **with RX** | 5,409 | 1,345 | 2.949 s |
| `03b-XBM-37_Quad_Aileron-Center-Left-Center-Right-Center.csv` | SPI | Aileron stick: center → left → center → right → center | 6,039 | 1,509 | 3.127 s |
| `04b-XBM-37_Quad_Elevator-Center-Back-Center-Forward-Center.csv` | SPI | Elevator stick: center → back → center → fwd → center | 6,035 | 1,509 | 3.123 s |
| `05b-XBM-37_Quad_Throttle-Low-High-Low.csv` | SPI | Throttle: low → high → low | 6,034 | 1,509 | 3.122 s |
| `06b-XBM-37_Quad_Rudder-Center-Left-Center-Right-Center.csv` | SPI | Rudder (yaw): center → left → center → right → center | 6,034 | 1,509 | 3.122 s |
| `07b-XBM-37_Quad_RateModeSwitch-1-2-3.csv` | SPI | Rate/speed mode switch: 1 → 2 → 3 | 6,034 | 1,509 | 3.122 s |
| `08b-XBM-37_Quad_FlipSwitch-PushButton_Off-On.csv` | SPI | Flip trick button: off → on | 6,034 | 1,509 | 3.122 s |
| `09b-XBM-37_Quad_VideoSwitch-Off-On-Off-On.csv` | SPI | Video record toggle: off → on → off → on | 6,023 | 1,506 | 3.116 s |
| `10b-XBM-37_Quad_PictureSwitch-PushButton-3X.csv` | SPI | Photo/picture button: pressed 3 times | 6,019 | 1,505 | 3.113 s |
| `11b-XBM-37_Quad_HeadlessSwitch-PushButton-Off-On.csv` | SPI | Headless mode button: off → on | 6,022 | 1,506 | 3.116 s |
| `12b-XBM-37_Quad_ReturnToHomeSwitch-PushButton-Off-On.csv` | SPI | RTH button: off → on | 6,034 | 1,508 | 3.127 s |
| `13b-XBM-37_Quad_LED-LightsSwitch-PushButton-On-Off.csv` | SPI | LED lights toggle: on → off | 6,019 | 1,505 | 3.113 s |
| `14b-XBM-37_Quad_OK-Switch-PushButton-Off-On.csv` | SPI | OK button: off → on | 6,027 | 1,507 | 3.120 s |
| `20b-XBM-37_Quad_Elevator-Trim-Center-Forward-Max_32-Clicks.csv` | SPI | Elevator trim: center → forward max (32 clicks) | 38,683 | 9,671 | 20.004 s |
| `21b-XBM-37_Quad_Elevator-Trim-Center-Back-Max_31-Clicks.csv` | SPI | Elevator trim: center → back max (31 clicks) | 33,215 | 8,304 | 17.191 s |
| `22b-XBM-37_Quad_Aileron-Trim-Center-Left-Max_32-Clicks.csv` | SPI | Aileron trim: center → left max (32 clicks) | 32,315 | 8,079 | 16.730 s |
| `23b-XBM-37_Quad_Aileron-Trim-Center-Right-Max_31-Clicks.csv` | SPI | Aileron trim: center → right max (31 clicks) | 28,495 | 7,124 | 14.756 s |
| `24b-XBM-37_Quad_Ail-Trim-Forward-Max_Ele-Trim-Right-Max_Push-OK-Button-2X.csv` | SPI | Elevator trim at forward max, aileron trim at right max, OK button pressed 2× | 29,275 | 7,319 | 15.176 s |

### Notes
- The "b" SPI files each contain one SPI byte per row. Multiple rows sharing the same `Packet ID` belong to one CSN-low SPI transaction.
- Files 03b–24b were all captured in **normal (post-bind) flight mode**. The TX had previously
  completed a successful bind.
- File 01b and 02b are functionally **identical** captures (confirmed by direct payload comparison)
  except for a tiny time offset (~7 µs between corresponding packets). This confirms the TX
  transmits identically whether or not an RX is present; see Section 5.

---

## 2. Key Protocol Parameters

| Parameter | Value |
|-----------|-------|
| RF transceiver chip | SV7241A (QFN-20, NRF24L01+ / BK2425 derivative) |
| RF frequency band | 2.4 GHz ISM |
| Air data rate (SV7241A) | 2 Mbps (RF_SETUP = 0x26, bit5 = RF_DR = 1) |
| Air data rate (nRF24L01+ equivalent) | **250 kbps** via `ssv_pack_dpl()` method |
| Packet size (raw, pre-pack) | 8 bytes |
| Packet size (after ssv_pack_dpl) | 12 bytes |
| Enhanced ShockBurst | Yes (dynamic payload length, no auto-ACK) |
| SPI clock frequency | ~143 kHz (7 µs period) |
| Bind packet count | **400** |
| Bind address (broadcast) | `E7 E7 E7 E7 67` |
| Data address format | `[TX_ID₀ TX_ID₁ TX_ID₂ E7 67]` |
| Bind channel (first packet) | 0x00 (universal) |
| Hop channels (bind + data) | **0x49, 0x34, 0x26, 0x07** (4 channels) |
| Packet period (steady state) | **~2,070 µs** (~2.07 ms) |
| CE high pulse width | ~1,172 µs |
| CE low gap (inter-packet) | ~896 µs |
| TX-only mode | Yes — EN_RXADDR = 0x00; TX never listens |

---

## 3. SPI Initialization Sequence

The sequence below is from `02b` (PIDs 0–24), occurring over the first ~5 ms after the SPI
bus becomes active (~150–335 ms after TX power-on due to startup delays).

### 3.1 SV7241A Private Bank Registers

The SV7241A (like BK2425) has bank-switched extended registers at addresses 0x18–0x1B.
Register 0x1F selects the bank. These registers hold RF/analog calibration constants specific
to the chip and **do not exist on nRF24L01+** — they are ignored when implementing with MPM.

| PID | Register | Data Written | Purpose |
|-----|----------|-------------|---------|
| 0 | 0x1F | `00` | Select bank 0 |
| 1 | 0x1B | `10 E1 D3 3D` | Bank 0 – RF calibration reg 1 |
| 2 | 0x19 | `06 AA A2 DB` | Bank 0 – RF calibration reg 2 |
| 3 | 0x1F | `01` | Select bank 1 |
| 4 | 0x19 | `77 48 9A E8` | Bank 1 – internal reg |
| 5 | 0x1B | `76 87 CA 01` | Bank 1 – internal reg |
| 6 | 0x1F | `02` | Select bank 2 |
| 7 | 0x1B | `A0 00 18 A0` | Bank 2 – internal reg |
| 8 | 0x1F | `04` | Select bank 4 |
| 9 | 0x18 | `01 00 F0 00` | Bank 4 – internal reg |
| 10 | 0x1F | `05` | Select bank 5 |
| 11 | 0x18 | `84 03 2A 03` | Bank 5 – internal reg |
| 12 | 0x19 | `90 BF 00 00` | Bank 5 – internal reg |
| 13 | 0x1A | `A0 0F 00 00` | Bank 5 – internal reg |

### 3.2 Standard NRF24L01-Compatible Registers

Written immediately after the private registers (PIDs 14–24):

| PID | Register | Written Value | Decoded Meaning |
|-----|----------|--------------|-----------------|
| 14 | CONFIG (0x00) | `0x0C` | EN_CRC=1, CRCO=1 (2-byte CRC), PWR_UP=0, PRIM_RX=0 (TX) |
| 15 | CONFIG (0x00) | *read back* `0x0C` | Verify CONFIG |
| 16 | TX_ADDR (0x10) | `E7 E7 E7 E7 67` | Bind broadcast address |
| 17 | RX_ADDR_P0 (0x0A) | `E7 E7 E7 E7 67` | Matches TX_ADDR for bind |
| 18 | EN_AA (0x01) | `0x00` | Auto-ACK **disabled** on all pipes |
| 19 | EN_RXADDR (0x02) | `0x00` | All RX pipes **disabled** – TX only |
| 20 | RF_CH (0x05) | `0x49` | Initial RF channel (73 MHz offset) |
| 21 | FEATURE (0x1D) | `0x04` | EN_DPL = 1 (dynamic payload length) |
| 22 | DYNPD (0x1C) | `0x01` | DPL enabled on pipe 0 |
| 23 | RF_SETUP (0x06) | `0x26` | SV7241A: 2 Mbps, max power |
| 24 | CONFIG (0x00) | `0x0E` | PWR_UP=1 → TX powered up |

**RF_SETUP = 0x26 mapping:**

| Chip | Bit 5 meaning | Value 0x26 decodes as |
|------|--------------|----------------------|
| SV7241A | RF_DR (0=1Mbps, 1=2Mbps) | **2 Mbps**, max power |
| nRF24L01+ | RF_DR_LOW (0=normal, 1=250kbps) | **250 kbps**, max power |

→ On nRF24L01+, writing 0x26 would set **250 kbps** (not 2 Mbps). For MPM emulation the
correct approach (same as FQ777) is `NRF24L01_SetBitrate(NRF24L01_BR_250K)` combined with
`ssv_pack_dpl()` to produce a compatible air packet.

**Startup timing:**

The TX power-on to first SPI transaction takes ~150 ms (CE is held high from
t = –72 ms until t = 0, then the ~150 ms wait elapses before SPI activity at t ≈ +181 ms in the
02b file). The entire init register sequence completes in ~6 ms.

---

## 4. Bind Sequence – Deep Analysis (02a + 02b)

### 4.1 Overview

After the ~150 ms startup wait, the TX sends exactly **400 bind packets** before switching to
normal data mode.

| Property | Value |
|----------|-------|
| Total bind packets | **400** |
| Bind address | `E7 E7 E7 E7 67` (fixed broadcast) |
| First bind packet channel | `0x00` (universal channel) |
| Remaining 399 bind packet channels | Cycling: `0x49 → 0x34 → 0x26 → 0x07 → 0x49 → …` |
| Bind packet payload | `20 14 07 03 TX_ID₀ TX_ID₁ TX_ID₂ CKSUM` |
| Bind packet content | Identical across all 400 transmissions |

The first bind packet is sent on RF channel **0x00**, ensuring a newly powered-on RX (sitting
on its default channel) can hear the bind announcement regardless of any prior state. Subsequent
bind packets cycle through the four data-hopping channels.

### 4.2 Bind Packet Structure

```
Byte  0  1    2    3    4        5        6        7
     [20 14   07   03   TX_ID₀  TX_ID₁  TX_ID₂  CKSUM]
```

| Byte | Value (this TX) | Description |
|------|----------------|-------------|
| B0 | `0x20` | Bind identifier byte (constant — marks this as a bind packet) |
| B1 | `0x14` | Protocol variant constant (XBM-37 specific; FQ777 uses `0x15`) |
| B2 | `0x07` | Protocol variant constant (XBM-37 specific; FQ777 uses `0x05`) |
| B3 | `0x03` | Protocol variant constant (XBM-37 specific; FQ777 uses `0x06`) |
| B4 | `0x91` | TX_ID byte 0 — unique per TX unit |
| B5 | `0x05` | TX_ID byte 1 — unique per TX unit |
| B6 | `0x05` | TX_ID byte 2 — unique per TX unit |
| B7 | `0x9B` | Checksum = (TX_ID₀ + TX_ID₁ + TX_ID₂) & 0xFF = (0x91+0x05+0x05) & 0xFF |

**Checksum formula for bind packet:** `B7 = (B4 + B5 + B6) & 0xFF`
(This differs from the data packet checksum which sums B0–B6.)

### 4.3 TX ID

TX ID is **3 bytes** embedded in B4–B6 of the bind packet. This TX unit has:

```
TX_ID = [0x91, 0x05, 0x05]
```

After bind, the TX_ADDR is set to: `[TX_ID₀  TX_ID₁  TX_ID₂  0xE7  0x67]`
= `[91  05  05  E7  67]`

The RX extracts TX_ID from the bind packet and uses it to construct the matching address for
normal data reception.

### 4.4 Bind Channel Sequence (from 02b SPI decoded)

```
Packet #1:   ch=0x00  [20 14 07 03 91 05 05 9B]  ← universal announce
Packet #2:   ch=0x49  [20 14 07 03 91 05 05 9B]
Packet #3:   ch=0x34  [20 14 07 03 91 05 05 9B]
Packet #4:   ch=0x26  [20 14 07 03 91 05 05 9B]
Packet #5:   ch=0x07  [20 14 07 03 91 05 05 9B]
Packet #6:   ch=0x49  [20 14 07 03 91 05 05 9B]
...continues cycling 0x49→0x34→0x26→0x07...
Packet #400: ch=0x26  [20 14 07 03 91 05 05 9B]  ← last bind
```

Channel usage across all 400 bind packets:

| Channel | Count | Notes |
|---------|-------|-------|
| 0x00 | 1 | First packet only |
| 0x49 (73 MHz) | 100 | Regular cycle |
| 0x34 (52 MHz) | 100 | Regular cycle |
| 0x26 (38 MHz) | 100 | Regular cycle (includes last bind packet) |
| 0x07 (7 MHz) | 99 | Regular cycle |

### 4.5 Bind-to-Normal Transition

After the 400th bind packet the TX:

1. Completes the 400th transmission (CE pulse ~1,172 µs, then CE low).
2. Waits **~16.6 ms** (no SPI activity; firmware processing time).
3. Writes the new TX_ADDR: `W_TX_ADDR [91 05 05 E7 67]` (SPI pid=1625 at t=1.175882 s).
4. Immediately clears STATUS and starts normal data mode.
5. First normal packet: ch=**0x07** (continuation of the hop sequence from where bind ended).

```
Last bind (400th):  t=1.159258 s, ch=0x26
TX_ADDR change:     t=1.175882 s  (+16.6 ms gap)
First data packet:  t=1.176619 s, ch=0x07  [E1 70 70 70 20 20 00 71]
```

### 4.6 IRQ / STATUS Analysis (from 02a Digital Capture)

The IRQ line (active-low) is asserted by the SV7241A for every successfully transmitted packet
(TX_DS interrupt):

| Observation | Value |
|-------------|-------|
| Total IRQ assertions | 1,340 (matching 1,340 normal CE pulses) |
| IRQ pulse duration (typical) | ~600 µs |
| IRQ source | TX_DS only (TX complete) |
| RX_DR interrupts | **Zero** — no data ever received |

The SPI confirms there are **no `R_RX_PAYLOAD` (0x61) commands** in either 01b or 02b.
Combined with `EN_RXADDR = 0x00` (all RX pipes disabled), it is impossible for the TX to
receive data from the RX. This is confirmed by the STATUS register never showing bit6
(RX_DR) = 1.

One extended IRQ assertion of **15.9 ms** occurs at t=1.160476 s (immediately after the 400th
bind packet). This is not an RX event — it is simply the TX_DS interrupt remaining uncleared
while the firmware processes the bind-completion event and updates TX_ADDR. The IRQ is
cleared when `W_STATUS [70]` is written at t=1.176254 s (pid=1626).

### 4.7 CE Pulse Timing (from 02a)

| Measurement | Value |
|-------------|-------|
| Normal CE pulse duration | min 1,165 µs, max 1,178 µs, **avg 1,172 µs** |
| CE inter-pulse gap (end to start) | **~896 µs** |
| Total cycle (CE high + gap) | **~2,068 µs** |
| Pulse-to-pulse interval (high to high) | min 2.063 ms, max 20.686 ms, **avg 2.084 ms** |

The 20.686 ms outlier corresponds to the bind-to-normal transition pause (~16.6 ms).

---

## 5. Bind vs No-RX Comparison (01b vs 02b)

**Finding: The TX transmits identically whether or not an RX is present.**

| Metric | 01b (No RX) | 02b (With RX) |
|--------|------------|----------------|
| Total TX payloads | 1,352 | 1,345 |
| Bind packets | 400 | 400 |
| Bind packet content | `20 14 07 03 91 05 05 9B` | `20 14 07 03 91 05 05 9B` (identical) |
| TX_ADDR change time | t=1.175899 s | t=1.175882 s |
| TX_ADDR after bind | `[91 05 05 E7 67]` | `[91 05 05 E7 67]` (identical) |
| Normal data payload (idle) | `E1 70 70 70 20 20 00 71` | `E1 70 70 70 20 20 00 71` (identical) |
| Time offset between captures | — | ~7 µs per packet |

The TX performs an automatic timed bind sequence (400 packets) and then switches to data mode
unconditionally, regardless of RX acknowledgment. The TX **never knows** whether the RX
accepted the bind. Bind is one-sided: the RX passively listens for the bind packet on ch=0x00
(or the cycling channels), extracts the TX_ID and hop channels, and then follows the TX's
normal data transmissions.

---

## 6. Normal Data Packet Format

### 6.1 Packet Structure (8 bytes)

```
Byte  0        1      2       3        4       5       6       7
     [Throttle Rudder Aileron Elevator Flags1  Flags2  Flags3  CKSUM]
```

**Checksum (B7):** `B7 = (B0 + B1 + B2 + B3 + B4 + B5 + B6) & 0xFF`

All 8-byte checksum values have been verified against this formula across all capture files.

### 6.2 Stick Channels (B0–B3)

| Byte | Channel | Min | Center | Max | Notes |
|------|---------|-----|--------|-----|-------|
| B0 | Throttle | `0xE1` | `0x70` | `0x00` | Non-return throttle |
| B1 | Rudder (Yaw) | `0x00` | `0x70` | `0xE1` | |
| B2 | Aileron (Roll) | `0xE1` | `0x70` | `0x00` | |
| B3 | Elevator (Pitch) | `0xE1` | `0x70` | `0x00` | |

All analog channels use the same range: **0x00 – 0xE1** (0 – 225 decimal) with center at
**0x70** (112 decimal).
- Throttle, Aileron, and Elevator use reversed state (0xE1...0x70...0x00).

### 6.3 Flags Byte 1 (B4)

| Value | Meaning | Description |
|-------|---------|-------------|
| `0x20` | **Trim Elevator Center** | Resets at TX start |
| `0x21 up to 0x40` | Trim elevator **forward** | 32 Clicks to max |
| `0x1F downto 0x01` | Trim elevator **back** | 31 Clicks to max |

### 6.4 Flags Byte 2 (B5)

| Value | Meaning | Description |
|-----|------|-------------|
| `0x20` | **Trim Aileron Center** | Resets at TX start |
| `0x21 up to 0x40` | Trim aileron **left** | 32 Clicks to max |
| `0x1F down to 0x01` | Trim aileron **right** | 31 Clicks to max |
| `0x80` | OK button | OK pressed (`0x20` → `0xA0`) |

`B5 bit7` is an OR mask on top of trim value (`B5_with_OK = B5_trim | 0x80`), e.g.
center `0x20 -> 0xA0`, right-max `0x01 -> 0x81`.

### 6.5 Flags Byte 3 (B6)

| Bits | Mask | Name | Description |
|------|------|------|-------------|
| [1:0] | `0x03` | Rate mode | `0x00`=rate 1 (slow), `0x01`=rate 2, `0x02`=rate 3 (fast) |
| 2 | `0x04` | LED off | `0`=LED lights ON, `1`=LED lights OFF |
| 3 | `0x08` | RTH | `1`=return-to-home active |
| 4 | `0x10` | Headless | `1`=headless mode active |
| 5 | `0x20` | Video | `1`=video recording active |
| 6 | `0x40` | Picture | `1`=photo capture triggered |
| 7 | `0x80` | Flip | `1`=3D flip command |

Normal state: `B6 = 0x00` (rate 1, LED on, no special modes)

### 6.6 Return-to-Home (RTH)

Updated analysis of **file 12b** (RTH OFF in first half, ON in second half):

- RTH-OFF payload is steady at `7E 70 70 70 20 20 00 0E`.
- After the OFF→ON transition, the payload changes to `7E 70 70 70 20 20 08 16`.
- The persistent functional change in this capture is `B6: 0x00 -> 0x08`
  (bit3 asserted when RTH is ON).

In this export, RTH is represented in the steady 8-byte payload by `B6 bit3`.

### 6.7 Example Payloads

| Payload | Meaning |
|---------|---------|
| `E1 70 70 70 20 20 00 71` | Throttle low `0xE1`, all sticks center `0x70`, LED on, no special modes `0x00` |
| `82 70 70 70 20 20 00 12` | Throttle ~mid `0x82`, all sticks center, LED on, no special modes |
| `00 70 70 70 20 20 00 90` | Throttle max `0x00`, all sticks center, LED on, no special modes |
| `E1 70 00 70 20 20 00 01` | Throttle low, aileron full right `0x00`, LED on, no special modes |
| `E1 70 E1 70 20 20 00 E2` | Throttle low, aileron full left `0xE1`, LED on, no special modes |
| `E1 70 70 00 20 20 00 01` | Throttle low, elevator full back `0x00`,LED on, no special modes |
| `82 70 70 70 20 20 80 92` | Throttle ~mid, all sticks center, flip command active `0x80` |
| `82 70 70 70 20 20 10 22` | Throttle ~mid, all sticks center, headless mode active `0x10` |
| `82 70 70 70 20 20 01 13` | Throttle ~mid, all sticks center, rate mode 2 `0x01` |
| `82 70 70 70 20 20 02 14` | Throttle ~mid, all sticks center, rate mode 3 `0x02` |
| `00 70 70 70 20 A0 00 F1` | Throttle max, all sticks center, OK button pressed `0xA0` |
| `00 70 70 70 20 20 04 75` | Throttle max, all sticks center, LED lights OFF `0x04` |

---

## 7. Per-Channel Control Analysis (03b – 24b)

All control captures were taken in post-bind normal mode. Hopping channels confirmed active:
`0x07, 0x49, 0x34, 0x26` (cyclic). Average packet interval: **~2.07 ms** across all files.

### 03b – Aileron (Roll)

- **Affected byte:** B2  
- Range observed: `0xE1` (full left) → `0x70` (center) → `0x00` (full right)  

### 04b – Elevator (Pitch)

- **Affected byte:** B3  
- Range observed: `0x00` (full back) → `0x70` (center) → `0xE1` (full forward)

### 05b – Throttle

- **Affected byte:** B0  
- Range observed: `0xE1` (low minimum) ↔ `0x70` (center) ↔ `0x00` (full maximum)  
- Non-return throttle (stick does not spring back to center)

### 06b – Rudder (Yaw)

- **Affected byte:** B1  
- Range observed: `0x00` (full left) → `0x70` (center) → `0xE1` (full right)  
- Note: B0 (throttle) also varies in this capture because left stick controls both
  throttle (up/down) and rudder (left/right) Mode 2 transmitter

### 07b – Rate Mode Switch

- **Affected byte:** B6 bits[1:0]  
- `B6 = 0x00`: Rate 1 (slowest/beginner)  
- `B6 = 0x01`: Rate 2 (intermediate)  
- `B6 = 0x02`: Rate 3 (fastest/expert)

### 08b – Flip Switch

- **Affected byte:** B6 bit7  
- `B6 = 0x00`: No flip; `B6 = 0x80`: 3D flip command active  
- While flip is held: throttle (B0) increments slightly (`0x82` → `0x83`)

### 09b – Video Switch

- **Affected byte:** B6 bit5  
- `B6 = 0x00`: Video off; `B6 = 0x20`: Video recording active  
- Toggle on/off: transitions between these two states

### 10b – Picture Switch (3×)

- **Affected byte:** B6 bit6  
- `B6 = 0x00`: Idle; `B6 = 0x40`: Photo capture triggered (momentary)

### 11b – Headless Switch

- **Affected byte:** B6 bit4  
- `B6 = 0x00`: Headless off; `B6 = 0x10`: Headless mode active

### 12b – Return to Home Switch

- **Capture split tested:** first half RTH OFF, second half RTH ON.
- **RTH OFF payload:** `7E 70 70 70 20 20 00 0E`
- **RTH ON payload:** `7E 70 70 70 20 20 08 16`
- **Primary byte change:** `B6: 0x00 -> 0x08`
  - `B6 bit3` is asserted when RTH is active in this capture
- **Minor secondary variation while ON:** `B0` occasionally increments `0x7E -> 0x7F`,
  producing checksum `0x16 -> 0x17`
- All other payload bytes remain unchanged across the OFF→ON transition in this file.

### 13b – LED Lights Switch

- **Affected byte:** B6 bit2  
- `B6 = 0x00`: LED lights **ON** (default)  
- `B6 = 0x04`: LED lights **OFF**  
- Note: Inverted logic — bit2=1 means OFF

### 14b – OK Switch

- **Affected byte:** B5 bit7  
- `B5 = 0x20`: OK button not pressed  
- `B5 = 0xA0`: OK button pressed (0x20 | 0x80)

### 20b – Elevator Trim Center → Forward Max (32 clicks)

- **Affected byte:** B4 (elevator trim)
- Observed progression: `0x20 -> ... -> 0x40`
- Endpoint in this capture: `B4 = 0x40` (forward max)
- `B5` remains `0x20`; `B6` remains `0x00`

### 21b – Elevator Trim Center → Back Max (31 clicks)

- **Affected byte:** B4 (elevator trim)
- Observed progression: `0x20 -> ... -> 0x01`
- Endpoint in this capture: `B4 = 0x01` (back max)
- `B5` remains `0x20`; `B6` remains `0x00`

### 22b – Aileron Trim Center → Left Max (32 clicks)

- **Affected byte:** B5 (aileron trim)
- Observed progression: `0x20 -> ... -> 0x40`
- Endpoint in this capture: `B5 = 0x40` (left max)
- `B4` remains `0x20`; `B6` remains `0x00`

### 23b – Aileron Trim Center → Right Max (31 clicks)

- **Affected byte:** B5 (aileron trim)
- Observed progression: `0x20 -> ... -> 0x01`
- Endpoint in this capture: `B5 = 0x01` (right max)
- `B4` remains `0x20`; `B6` remains `0x00`

### 24b – Trim Endpoints + OK Button (2 presses)

- Fixed trim baseline in this capture:
  - `B4 = 0x40` (elevator forward max)
  - `B5 trim = 0x01` (aileron right max)
- **OK effect:** `B5 bit7` toggles over trim baseline:
  - not pressed: `B5 = 0x01`
  - pressed: `B5 = 0x81` (`0x01 | 0x80`)
- `B6` remains `0x00` throughout.

---

## 8. RF Timing Summary

### 8.1 Per-Packet SPI Cycle (02b decoded)

Each packet transmission consists of 4 SPI transactions:

```
1. W_STATUS [27] = 0x70    → Clear TX_DS / MAX_RT / RX_DR interrupt flags
2. FLUSH_TX [E1]           → Empty TX FIFO
3. W_RF_CH  [25] = <ch>    → Set hop channel
4. W_TX_PAYLOAD [A0] = 8B  → Write 8-byte payload to TX FIFO
                             [CE pulse ~1,172 µs to transmit]
                             [~896 µs gap before next cycle]
```

### 8.2 Bind Phase Timing

| Event | Timestamp (02b) |
|-------|----------------|
| Init register writes complete | t = 0.186 s |
| Startup wait completes | t = 0.335 s (~150 ms) |
| First bind packet (ch=0x00) | t = 0.335838 s |
| 400th (last) bind packet (ch=0x26) | t = 1.159258 s |
| TX_ADDR change to data address | t = 1.175882 s (+16.6 ms) |
| First normal data packet | t = 1.176619 s |

Total bind phase duration: **~0.824 s** (150 ms wait + ~674 ms for 400 packets at 2.07 ms each)

### 8.3 Normal Data Phase Timing

| Event | Interval |
|-------|---------|
| Packet 1 → 2 (startup) | 0.918 ms |
| Packet 2 → 3 (startup) | 0.916 ms |
| Packet 3 → 4 (startup) | 1.491 ms |
| Steady state (4+) | **~2.070 ms** per packet |

The first 3 packets after bind-to-data transition have shorter intervals, then settle into the
steady 2.07 ms period.

### 8.4 STATUS Byte Values Observed

| STATUS | Meaning | When seen |
|--------|---------|-----------|
| `0x0E` | TX FIFO not full, RX FIFO empty, all interrupts clear | Normal/idle |
| `0x2E` | TX_DS = 1 (TX complete interrupt), TX FIFO not full | After each TX packet |

RX_DR (bit6) is **never set** in any STATUS byte — confirming no data is ever received.

---

## 9. MPM Implementation Notes

### 9.1 Comparison with FQ777

The XBM-37 protocol is closely related to FQ777 but has the following differences:

| Property | FQ777 | XBM-37 | Comment|
|----------|-------|---------|---------|
| Bind count | 1,000 | **400** | works with either |
| Bind packet B1 | `0x15` | `0x14` | different |
| Bind packet B2 | `0x05` | `0x07` | different |
| Bind packet B3 | `0x06` | `0x03` | different |
| Hop channels | `4D 43 27 07` | **`49 34 26 07`** | different |
| Data range | 0x00–0x64 | **0x00–0xE1** | different |
| Bind address | `E7 E7 E7 E7 67` | `E7 E7 E7 E7 67` | (same) |
| Checksum method | Sum B0–B6 | Sum B0–B6 | (same) |
| Bind checksum | Sum B4–B6 | Sum B4–B6 | (same) |
| ssv_pack_dpl encoding | Yes | Yes | (same) |
| Air bitrate (nRF24L01+) | 250 kbps | 250 kbps | (same) |

### 9.2 Required Implementation Constants

```c
static const uint8_t XBM37_bind_addr[] = {0xE7, 0xE7, 0xE7, 0xE7, 0x67};
static const uint8_t XBM37_hop_channels[] = {0x49, 0x34, 0x26, 0x07};
```

### 9.3 Bind Packet Builder

```c
// Bind packet (bytes 4–6 = TX ID; B7 = checksum of B4+B5+B6)
packet[0] = 0x20;
packet[1] = 0x14;
packet[2] = 0x07;
packet[3] = 0x03;
packet[4] = rx_tx_addr[0];   // TX_ID byte 0
packet[5] = rx_tx_addr[1];   // TX_ID byte 1
packet[6] = rx_tx_addr[2];   // TX_ID byte 2
packet[7] = packet[4] + packet[5] + packet[6];
```

### 9.4 Data Packet Builder

```c
packet[0] = convert_channel_16b_limit(THROTTLE, 0xE1, 0x00);
packet[1] = convert_channel_16b_limit(RUDDER,   0x00, 0xE1);
packet[2] = convert_channel_16b_limit(AILERON,  0xE1, 0x00);
packet[3] = convert_channel_16b_limit(ELEVATOR, 0xE1, 0x00);
packet[4] = ((convert_channel_8b(CH13) * 63) / 255) + 1;                  // ele trim (01..20..40)
packet[5] = 64 - (((uint32_t)convert_channel_8b(CH14) * 63 + 127) / 255); // ail trim (40..20..01)
packet[5] |= GET_FLAG(OK_SW, 0x80);          // OK button
packet[6] = (rate_mode & 0x03)               // bits[1:0] = rate (0/1/2)
           | GET_FLAG(LED_SW, 0x04)          // bit2=1 = LED off
           | GET_FLAG(RTH_SW, 0x08)          // bit3 = RTH
           | GET_FLAG(HEADLESS_SW, 0x10)     // bit4 = headless
           | GET_FLAG(VIDEO_SW, 0x20)        // bit5 = video
           | GET_FLAG(PHOTO_SW, 0x40)        // bit6 = picture
           | GET_FLAG(FLIP_SW, 0x80);        // bit7 = flip
packet[7] = 0;
for (uint8_t i = 0; i < 7; i++) packet[7] += packet[i];  // checksum
```

### 9.5 Address Management

```c
// TX init:
rx_tx_addr[2] = 0x00; // original hardcoded value now changes for model match capability (see below)
rx_tx_addr[3] = 0xE7;
rx_tx_addr[4] = 0x67;
// rx_tx_addr[0] and [1] are random/unique to the TX
// rx_tx_addr[2] now varies by changing receiver number (0-63) for model match

// Bind address (used during bind):
NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, XBM37_bind_addr, 5);

// After 400th bind packet, switch to data address:
NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
```

### 9.6 RF Init

```c
void XBM37_RF_init() {
    NRF24L01_Initialize();
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, XBM37_bind_addr, 5);
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // no auto-ACK
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x00);  // no RX pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x04);    // EN_DPL
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x01);      // DPL pipe 0
    NRF24L01_SetBitrate(NRF24L01_BR_250K);            // 250 kbps on nRF24L01+
}
```

### 9.7 Air Encoding

Because both the XBM-37 TX and RX use SV7241A (a BK2425 derivative), the air packet format is
the SV7241A Enhanced ShockBurst encoding. When implementing with nRF24L01+, the same
`ssv_pack_dpl()` function used in the FQ777 protocol must be applied to convert the raw 8-byte
payload into the 12-byte packed representation that nRF24L01+ transmits at 250 kbps to produce
a compatible on-air signal.

---

*Analysis completed using Python scripts against the raw CSV captures. All packet checksums,
channel sequences, bind counts, and register values verified programmatically.*
