# Frequency Tuning
Certain protocols which use the CC2500 RF module require fine-tuning the frequency for optimal performance with genuine receivers.  

The protocols which require frequency tuning are:
* **FrSkyD** (e.g. FrSky D4R and D8R, DIY RX-F801 and RX-F802 receivers)
* **FrSkyV** (e.g. FrSky V8R4, V8R7 and V8FR receivers)
* **FrSkyX** (e.g. FrSky X4R, X6R, X8R, and XSR receivers)
* **S-FHSS** (e.g. Futaba S-FHSS receivers)

There is a [video](#video) at the end of this page which gives an example of the tuning process.

## More information
Original FrSky and Futaba receivers have been frequency-tuned by the manufacturer at the factory.  Because of variations in the oscillator crystals used in multiprotocol modules it is necessary to fine-tune the module to match the manufacturer frequencies.  

'Compatible' receivers suffer the same variation in crystal oscillators as multiprotocol modules, but have to be compatible with genuine (manufacturer-tuned) transmitters so they will typically have auto-tuning built in, and will self-tune to the radio's frequency when they are bound.

## Fine-tuning procedure
**Note:** For best results, the fine-tuning procedure should be carried out with a genuine FrSky or Futaba receiver.

The procedure can be performed in serial or PPM mode, but is easier with in serial mode where the effect of the change can be seen in real-time.

### Preparation
The radio needs to be bound with the receiver.
1. Configure the radio with the appropriate protocol
1. Set the **Freq** value to 0
1. Put the receiver into **Binding** mode
1. Attempt to bind the radio to the receiver

If the radio binds to the reciever, carry on.  If not, return to step 2. and change the **Freq** value to either **-40** or **40** and try to bind again.  If you still can't bind continue to try higher and lower values until the bind is successful.

### Fine tuning
**Tip:** If you have telemetry configured and a voice-capable radio, enable a voice alarm for telemetery loss so that you receive an immediate alert when the receiver connection is lost.

Once the radio is bound to the receiver:
1. Return to the **Freq** option
1. Lower the value until the radio loses the connection with the receiver.  Record the value (`TUNE_MIN`).
1. Raise the value so that the connection is restored, then continue to raise it until the radio loses the connection with the receiver again.  Record the value (`TUNE_MAX`).
1. Calculate the median between the two values
   `(TUNE_MIN + TUNE_MAX) / 2 = TUNE_MEDIAN`
1. Set **Freq** to the median value

#### Example 
Connection is lost at -73 and +35; the median is -19:

`(-73 + 35) / 2 = -19`

### Finally
Once the **Freq** value is known it should be applied to all other models which use this protocol and, if they were previously bound, the receivers must be re-bound.

For convenience this can be done in the `_Config.h` (or `_MyConfig.h`) configuration file.

#### Forced tuning values
Once known-good tuning values have been determined, they can be stored in the configuration file to be automatically applied to all models which use the given protocol.

These settings can also be used to force different tuning values for different multiprotocol modules, removing the need to alter the tuning option on the transmitter when swapping between modules. (Assuming that the modules also share a common hardware ID.)

**Note:** If a forced tuning value is set in the configuration, it cannot be overriden by the protocol's **Freq** option on the radio for any model.

```
/*******************************/
/*** CC2500 FREQUENCY TUNING ***/
/*******************************/
//For optimal performance the CC2500 RF module used by the FrSkyD, FrSkyV, FrSkyX, and SFHSS protocols needs to be tuned for each protocol.
//Initial tuning should be done via the radio menu with a genuine FrSky or Futaba receiver.  
//Once a good tuning value is found it can be set here and will override the radio's 'option' setting for all existing and new models which use that protocol.
//Valid range is -127 to +127
//Uncomment the lines below (remove the "//") and set an appropriate value (replace the "0") to enable.
//#define FORCE_FRSKYD_TUNING 0
//#define FORCE_FRSKYV_TUNING 0
//#define FORCE_FRSKYX_TUNING 0
//#define FORCE_SFHSS_TUNING  0
```

## Video
[![Frequency tuning video](https://img.youtube.com/vi/C483uNWwAaM/0.jpg)](https://www.youtube.com/watch?v=C483uNWwAaM)
