#Bluetooth Telemetry in PPM Mode
###Telemetry

There are 4 protocols supporting telemetry: Hubsan, DSM, FrSkyD and FrSkyX.

Hubsan displays the battery voltage and TX RSSI.

DSM displays TX RSSI and full telemetry.

FrSkyD displays full telemetry (A0, A1, RX RSSI, TX RSSI and Hub).

FrSkyX displays full telemetry (A1, A2, RX RSSI, TX RSSI and Hub).

### If used in PPM mode

Telemetry is available as a serial 9600 8 n 1 output on the TX pin of the Atmega328p using the FrSky hub format for Hubsan, FrSkyD, FrSkyX and DSM format for DSM2/X.

You can connect it to your TX if it is telemetry enabled or use a bluetooth adapter (HC05/HC06) along with an app on your phone/tablet ([app example](https://play.google.com/store/apps/details?id=biz.onomato.frskydash&hl=fr)) to display telemetry information and setup alerts.

### If used in Serial mode
Telemetry is built in for er9x and ersky9x TXs.

To enable telemetry on a Turnigy 9X or 9XR you need to modify your TX following one of the Frsky mod like this [one](http://blog.oscarliang.net/turnigy-9x-advance-mod/).

Note: DSM telemetry is not available on er9x due to a lack of flash space.

Enabling telemetry on a 9XR PRO and may be other TXs does not require any hardware modifications. The additional required serial pin is already available on the TX back module pins.

Once the TX is telemetry enabled, it just needs to be configured on the model (see er9x/ersky9x documentation).

