# Bluetooth Telemetry in PPM Mode

## Telemetry

There are 4 protocols supporting telemetry: Hubsan, DSM, FrSkyD and FrSkyX.

Hubsan displays the battery voltage and TX RSSI.

DSM displays TX RSSI and full telemetry.

FrSkyD displays full telemetry (A0, A1, RX RSSI, TX RSSI and Hub).

FrSkyX displays full telemetry (A1, A2, RX RSSI, TX RSSI and Hub).

## If used in PPM mode

Telemetry is available as a serial output on the TX pin of the Atmega328p using the FrSky hub format for Hubsan, FrSkyD, FrSkyX and DSM format for DSM2/X.  The serial paramets depends on the protocol:

Protocol|Serial Parameters
--------|-----------------
Hubsan|9600bps 8n1
FrSkyD|9600bps 8n1
FrSkyX|57,600bps 8n1
DSM2/X|125,000bps 8n1

The serial stream is also available on pin 5 of the Module connector (pins numbered from top to bottom) on the [4-in-1 module](Module_BG_4-in-1.md) and the [V2.3d modules](Module_Build_yourself_PCB.md) provided the Tx jumper has been soldered.  See the linked module documentation for what this means. 


You can connect it to your TX if it is telemetry enabled or use a bluetooth adapter (HC05/HC06) along with an app on your phone/tablet ([app example](https://play.google.com/store/apps/details?id=biz.onomato.frskydash&hl=fr)) to display telemetry information and setup alerts.


