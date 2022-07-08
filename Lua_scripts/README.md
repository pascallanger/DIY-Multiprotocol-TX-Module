# Multiprotocol TX Module OpenTX LUA scripts
<img align="right" width=300 src="../docs/images/multi.png" />

If you like this project and want to support further development please consider making a [donation](../docs/Donations.md).  

<table cellspacing=0>
  <tr>
    <td align=center width=200><a href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=VF2K9T23DRY56&lc=US&item_name=DIY%20Multiprotocol&currency_code=EUR&amount=5&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHosted"><img src="../docs/images/donate_button.png" border="0" name="submit" title="PayPal - Donate €5" alt="Donate €5"/></a><br><b>€5</b></td>
    <td align=center width=200><a href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=VF2K9T23DRY56&lc=US&item_name=DIY%20Multiprotocol&currency_code=EUR&amount=10&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHosted"><img src="../docs/images/donate_button.png" border="0" name="submit" title="PayPal - Donate €10" alt="Donate €10"/></a><br><b>€10</b></td>
    <td align=center width=200><a href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=VF2K9T23DRY56&lc=US&item_name=DIY%20Multiprotocol&currency_code=EUR&amount=15&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHosted"><img src="../docs/images/donate_button.png" border="0" name="submit" title="PayPal - Donate €15" alt="Donate €10"/></a><br><b>€15</b></td>
    <td align=center width=200><a href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=VF2K9T23DRY56&lc=US&item_name=DIY%20Multiprotocol&currency_code=EUR&amount=25&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHosted"><img src="../docs/images/donate_button.png" border="0" name="submit" title="PayPal - Donate €25" alt="Donate €25"/></a><br><b>€25</b></td>
    <td align=center width=200><a href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=VF2K9T23DRY56&lc=US&item_name=DIY%20Multiprotocol&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHosted"><img src="../docs/images/donate_button.png" border="0" name="submit" title="PayPal - Donate" alt="Donate"/></a><br><b>Other</b></td>
  </tr>
</table>

## MultiConfig

Enables to modify on a Multi module the Global ID, Cyrf ID or format the EEPROM.

Matching the ID of 2 Multi modules enable them to control the same receivers without rebinding. Be carefull the 2 modules should not be used at the same time unless you know what you are doing.

Notes:
- Supported from Multi v1.3.2.85 or above and OpenTX 2.3.12 or above
- The Multi module to be configured must be active, if there is a second Multi module in the radio it must be off
- Located on the radio SD card under \SCRIPTS\TOOLS

[![MultiCconfig](https://img.youtube.com/vi/lGyCV2kpqHU/0.jpg)](https://www.youtube.com/watch?v=lGyCV2kpqHU)

## MultiChannelsUpdater

Automatically name the channels based on the loaded Multi protocol and sub protocol including the module channel order convention.

Need OpenTX 2.3.9 or above. Located on the radio SD card under \SCRIPTS\TOOLS. This script needs MultiChan.txt to be present in the same folder.

[![MultiChannelsUpdater](https://img.youtube.com/vi/L58ayXuewyA/0.jpg)](https://www.youtube.com/watch?v=L58ayXuewyA)

## MultiLOLI

Script to set the channels function (switch, servo, pwm, ppm, sbus) on a [LOLI RX](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/blob/master/Protocols_Details.md#loli---82)

[![MultiLOLIconfig](https://img.youtube.com/vi/e698pQxfv-A/0.jpg)](https://www.youtube.com/watch?v=e698pQxfv-A)

## Graupner HoTT

Enable text configuration of the HoTT RX and sensors: Vario, GPS, ESC, GAM and EAM.

Need OpenTX 2.3.9 or above. Located on the radio SD card under \SCRIPTS\TOOLS.

Notes:
- Menu/MDL/Model is used to cycle through the detected sensors.
- It's normal to lose the telemetry feed while using the text mode configuration. Telemetry will resume properly if the script is exited by doing a short press on the exit button.

[![Text mode video](https://img.youtube.com/vi/81wd8NlF3Qw/0.jpg)](https://www.youtube.com/watch?v=81wd8NlF3Qw)

## Graupner HoTT Model Locator

This is the Graupner HoTT adapted version of the Model Locator script using RSSI.

The OpenTX sensor "RSSI" is populated by the individual OpenTX telemetry protocol implementations and returns a value from 0..100 (percent) originating from the early FrSky implementation. It turns out that FrSky did not really provide a genuine signal strength indicator in units of dbm but a link quality indicator in 0..100%. With Graupner HoTT the link quality indicator is not a good basis for the model locator as it is very non-linear and doesn't change much with distance. Using the Graupner HoTT telemetry sensor "Rssi" which is a true signal strength indicator serves the purpose of locating a model much better as it varies much more with distance.

## DSM Forward Programming

Navigation is mainly done using the scroll wheel and ENT. Short press on ENT will edit a value. When editing a value a long ENT press will restore the value to its default. To exit the script and terminate all current operations correctly short press RTN (if you don't do this the RX might not store the changes).

This is a work in progress. It's only available for color screens (Horus, TX16S, T16, T18...).

If some text appears as Unknown_xxx, please report xxx and what the exact text display should be.

Need OpenTX 2.3.10 nightly or above. Located on the radio SD card under \SCRIPTS\TOOLS.

[![DSM Forward Programming](https://img.youtube.com/vi/sjIaDw5j9nE/0.jpg)](https://www.youtube.com/watch?v=sjIaDw5j9nE)

## DSM PID Flight log gain parameters for Blade micros

Lua telemetry script from [feathering on RCGroups](https://www.rcgroups.com/forums/showpost.php?p=46033341&postcount=20728) to facilitate setting the Gain Parameters on the Blade 150S FBL. It doesn't use Forward Programming but instead it just reads telemetry data from the Multi-module and displays it on a telemetry display.

It is very similar to the Telemetry Based Text Generator functionality on Spektrum transmitters where one doesn't need to rely on the angle of the swashplate to determine selection/value.
