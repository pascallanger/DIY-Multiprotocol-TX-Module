# Channel Order

Channel ordering is extremely important to the Multiprotocol Module.  It is vital that the module receives channel data from the radio in the order which it expects so that it can reorder the data sent over the air into the order which the receiver requires/expects.

## Background
The Multiprotocol Module supports dozens of different protocols, many of which require channel data to be sent in a specific order (especially ones for models with integrated receiver/flight controllers).

The Multiprotocol Module was designed to:
* Enable users to control many different models or receivers using a single transmitter system
* Work with radios where the input channels could not be reassigned
* Make it simple to configure models without having to think about the channel order they require

To address these considerations the Multiprotocol Module will internally reorder the radio's output channels to the order required by the selected protocol before transmitting the data to the receiver.

The Multiprotocol Module also has functionality which depends on knowing which input channel is the throttle:
* 'Bind on Channel' requires the throttle to be at -100% before the bind can be initiated (so that bind cannot be initiated while flying)
* 'Throttle Kill' needs to know which channel to apply the throttle reduction to

**So, the Multiprotocol Module must know what order the channel data from the radio is in.  However, because the radio does not tell the module which input is assigned to each channel, the order is fixed in the Multiprotocol Module's firmware.  This is the _expected channel order_.**

## Expected Channel Order
The expected channel order is the order in which the Multiprotocol Module expects to receive channel data from the radio.  It associates the four primary radio inputs (aileron, elevator,throttle, and rudder) to the four output channels CH1, CH2, CH3, and CH4.  In AETR, **A**ileron is on CH1, **E**levator is on CH2, **T**hrottle is on CH3, and **R**udder is on CH4.

**The default expected channel order is AETR.**

The Multiprotocol Module uses the expected channel order to reorder channels into the order required by the selected protocol, and to apply channel-specific features such as 'Throttle Kill'.

If you are using firmware which has an expected channel order of AETR (the default), and you are using a protocol which has a fixed channel order†, you must create your radio models with a channel order of AETR, regardless of the channel order the receiver requires.  **The Multiprotocol Module will do the conversion from AETR to the required order for you.**

For example, Spektrum / DSM receivers expect **TAER**.  When you configure the model in the radio you set it up in **AETR** order, the Multiprotocol Module reorders the channels before transmission, the receiver receives **TAER**, everything works as expected:

Radio CH | Radio Input/Output | Module Reordering | Receiver Receives |
| --- | --- | --- | --- | 
| 1 | Ail | 3 -> 1 | Thr |
| 2 | Ele | 1 -> 2 | Ail |
| 3 | Thr | 2 -> 3 | Ele |
| 4 | Rud | 4 -> 4 | Rud |

If the radio channel order is not in the expected order (AETR) the channel reordering will be broken.

For example, if the radio is configured with TAER, but the Multiprotocol module expects AETR, this will happen:

Radio CH | Radio Input/Output | Module Reordering | Receiver Receives |
| --- | --- | --- | --- | 
| 1 | Thr | 3 -> 1 | Ele |
| 2 | Ail | 1 -> 2 | Thr |
| 3 | Ele | 2 -> 3 | Ail |
| 4 | Rud | 4 -> 4 | Rud |

With potentially dangerous or disasterous consequences.

At best, you won't be able to arm your model because the throttle value is too high (because the receiver sees the elevator output, which is at 50%, where it expects throttle to be).  At worst the throttle will go to 50% as soon as the model binds with the radio.

**†** Not all protocols have a fixed channel order, see [Protocols Without a Fixed Channel Order](#protocols-without-a-fixed-channel-order).

## Setting the Expected Channel Order
You can change the expected channel order by compiling or downloading firmware with a different channel order and flashing it to your Multiprotocol Module.

1. If you compile your own firmware, modify the channel order line in the `_Config.h` file to match your preference
2. If you flash the pre-compiled firmware from the Releases page, download the order-specific firmware image which matches your preference

**After updating the Multiprotocol Module you must still ensure your radio setup matches the module's expected order.**

## Changing the Channel Order on the radio
**Your radio is not aware of the channel order expected by the Multiprotocol Module, and the module is not aware of the default channel order on your radio.**

Once you have settled on a preferred channel order, and flashed your Multiprotocol Module with firmware which expects that order, you should:

1. Change the Default Channel Order setting on your radio to match the Multiprotocol Module
1. Ensure that any models which you control with the Multiprotocol Module are configured with the channel order which the Module expects

**Changing the Default Channel Order only affects new models - you must manually edit existing models.**

## Protocols Without a Fixed Channel Order
The Multiprotocol Module also supports protocols which _do not_ have a specific channel order.  For these protocols the channel order _is not_ changed and will be transmitted as received from the radio.

Examples of protocols which are not reordered: Corona, FrSkyD, FrSkyX, FrSkyV, Hitec, and WFly.

The full list of supported protocols, including the output channel order for each of them, is available [here](../Protocols_Details.md).

Where the channel table looks like this, with an input assigned to each channel, the output **is** being reordered:

CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

Where the channel table looks like this, the output **is not** being reordered:

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
