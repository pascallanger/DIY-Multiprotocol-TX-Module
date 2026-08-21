# MC8RE V2 (A7105)

This protocol supports the MC8RE V2 receiver family using an A7105 RF chip.
It was reconstructed from a wired A7105 SPI capture of the original
transmitter and then verified on two MC8RE V2 receivers.

## Implemented behaviour

- bind sequence: six packets on A7105 ID `5475C52A`, channel
  `01`, then normal operation on ID `51D567EB`;
- 16-channel hopping and the original startup/RX slot timing captured from
  the transmitter;
- eight 11-bit packed output channels, with EdgeTX +/-100% mapped to
  1000/1500/2000 us;
- telemetry A1 (receiver/BEC voltage) and A2 (external battery voltage,
  0.1 V per count; set EdgeTX A2 ratio to 25.5 V for a 4S range);
- a relative local A7105 RSSI indication.  It is not a calibrated received
  signal-strength measurement supplied by the receiver.

## Known limitation

The normal link ID and hopping sequence are intentionally fixed to the
capture-proven transmitter identity `51D567EB`.  Another original
transmitter has been observed to use a different ID and hopping sequence.
Deriving only the low 24 bits from the EdgeTX receiver number bound briefly
but did not maintain control, so it is deliberately not exposed as an option.
This is why this change is suitable for a **Draft PR / request for more
captures**, rather than a claim of universal MC8RE compatibility.

## Reproduction evidence

The contributor can provide the original Saleae `.sal` wired capture on
request.  Its SPI settings are CPOL=0, CPHA=0, MSB first; A7105 CS, SCK,
MOSI and MISO were captured.  No receiver serial output or third-party RF
packet decoder was used to infer the normal-channel format.
