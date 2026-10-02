# MC8RE V2 (A7105)

This protocol supports the MC8RE V2 receiver family using an A7105 RF chip.
It was reconstructed from a wired A7105 SPI capture of the original
transmitter and then verified on two MC8RE V2 receivers.

## Implemented behaviour

- bind sequence: six packets on A7105 ID `5475C52A`, channel
  `01`, then normal operation on ID `51D567EB`;
- 16-channel hopping and the original startup/RX slot timing captured from
  the transmitter;
- eight 11-bit packed output channels with piecewise scaling: EdgeTX
  -100%/0%/+100% maps to nominal PWM 1000/1500/2000 us (wire
  500/1000/1500). Extended -125%/+125% reaches nominal 500/2500 us
  (wire 0/2000);
- telemetry A1 (receiver/BEC voltage) and A2 (external battery voltage,
  0.1 V per count; set EdgeTX A2 ratio to 25.5 V for a 4S range);
- a relative local A7105 RSSI indication.  It is not a calibrated received
  signal-strength measurement supplied by the receiver.

## PWM mapping and receiver measurements

The v1.3.4.74 PWM mapping is based on Session 3 receiver CH1 measurements
with v1.3.4.73 (approximately 858/1523/2187 us at minimum/neutral/maximum).
Session 4 verified v1.3.4.74 CH1 at approximately 1000/1499–1500/1998 us.
The v1.3.4.76 extended-range test preserves the standard mapping within
+/-100% and stretches only the outer 100–125% segments to nominal
500–2500 us. A subsequent +/-125% capture measured receiver CH1 at
499.042/1500.125/2499.208 us (minimum/neutral/maximum). Median PWM period
was 14.001167 ms. This capture contained 538 complete pulses, including
58 stable minimum pulses and 44 stable maximum pulses. Logic D7 was
connected directly to receiver CH1; sampling rate was 24 MHz.

These are receiver CH1 PWM measurements. All eight channel fields use the
same encoder, but other receiver channels have not been measured individually.
The extended-range capture did not separately hold +/-100% positions;
those were measured on the preceding standard-range build. No simultaneous
SPI traffic was recorded in these PWM captures.

For the full range, enable EdgeTX Extended Limits, keep mixer weight at
100%, and set output limits to -125%/+125% with zero trim/subtrim/offset.
Confirm the channel monitor reaches +/-125%. MULTI serial input saturates
at that range. The outer 100–125% segments intentionally have a steeper
slope than the standard range.

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
