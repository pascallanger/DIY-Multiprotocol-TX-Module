#Model Setup
This is the page to document model or receiver specific setup instructions.

The Deviation project (on which this project was based) have a useful list of models [here](http://www.deviationtx.com/wiki/supported_models).

#Syma X5C
<img src="http://img2.cheapdrone.co.uk/images/upload/2014/12/X5C%203/SKU115108-7.jpg" Width="200" Height="200" />
##Channel Map
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|FLIP|RATES|PICTURE|VIDEO
##Binding
There are no special binding instructions.  The model powers up in Autobind mode and expects the bind sequence from the transmitter within the first 4-5 seconds. 
##Tx Setup
A basic 4-channel setup works perfectly, but some improvements are possible:   
###Setting up a switch to Flip    

1. Choose your "Rates" switch - typically the momentary TRN switch
1. In the Mixer create an entry for CH5
1. Edit this line as follows:  
   - er9X: Source: sTRN, Weight 100 (or whatever switch you selected)
   - OpenTx: Source: SH, Weight 200 (or whatever switch you selected)
   
###Setting up a swich for high rates

1. Choose your "Rates" switch
1. In the Mixer create an entry for CH6
1. Edit this line as follows:  
   - er9X: Source: sTHR, Weight 100 (or whatever switch you selected)
   - OpenTx: Source: SF, Weight 200 (or whatever switch you selected)

When the switch is in the rear position the rates will be standard, when the switch is forward rates will be high.  There is no need to move the throttle stick to the full up and full down position as with the standard controller. 

###Setting up Idle-up
One of the most annoying functions on the Syma X5C is that the motors stop when the throttle is pulled back.  This can be fixed by implmenting Idle-up on the transmitter (think of this as a very simple version of the Betaflight "Air Mode").  Idle up will ensure that even when the throttle is all the way down, a minimum command is passed to the motor to keep them spinning and to activate the stabilization.  

**To do this**:   

1. Decide on a switch you will use to activate Idle up
1. In the mixer menu add a line under Throttle and mix in a value of between 4 and 6 to be added to the throttle value if the switch is activated.  What this does is effectively prevents the throttle from going down to less than this value.
1. When you want to fly in "idle-up" mode flick the switch and your stabilization will always be active.  
1. Remeber to switch off idle-up as soon as the quad lands (or crashes - to avoid damage to the motors)

#Inductrix (Horizon Hobby)

<img src="https://s7d5.scene7.com/is/image/horizonhobby/BLH8700_a0" Width="200" Height="200" />

##Binding
For telemetry enabled modules, you should just let the remote autodetect the settings. Otherwise choose DSMX 22ms with 6ch or 7ch. To bind the model, keep the transmitter off, power on the Inductrix. Wait until it flashes fast and then power up the Tx and use Bind.

##Tx Setup
Remember that 100% on your transmitter using the MULTI-Module corresponds to 125% on the DSM receiver side.  On some functions sending 100% will confuse the model. Conversely 80% on your Tx is interpreted to be 100% at the model. Consider this when implementing the suggestions below.
Setup channel 6 with a momemtary button or switch (e.g. SH on the Taranis) and use that switch to switch between modes. Set the output to somewhere between 40% to 60% for best results.
For Inductrix FPV you might need to adjust the lower end of throttle to be a higher than default, otherwise motors will be spinning on minimal throttle.  One way to do this is to set the throttle to 80% output (100% of DSM output) and then to enable the **Throttle Idle Trim Only** under the Model Setup menu.   
