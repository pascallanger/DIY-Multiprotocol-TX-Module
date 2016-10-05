#Model Setup
This is the page to document model or receiver specific setup instructions.

The Deviation project (on which this project was based) have a useful list of models [here](http://www.deviationtx.com/wiki/supported_models).

#Syma X5C
<img src="http://img2.cheapdrone.co.uk/images/upload/2014/12/X5C%203/SKU115108-7.jpg" Width="200" Height="200" />
##Binding
There are no special binding instructions.  The model powers up in Autobind mode and expects the bind sequence from the transmitter within the first 4-5 seconds. 
##Tx Setup
A basic 4-channel setup works perfectly, but some improvements are possible:   
###Setting up a switch to Flip   

###Setting up a swich for high rates

###Setting up Idle-up
One of the most annoying functions on the Syma X5C is that the motors stop when the throttle is pulled back.  This can be fixed by implmenting Idle-up on the transmitter (think of this as a very simple version of the Betaflight "Air Mode").  Idle up will ensure that even when the throttle is all the way down, a minimum command is passed to the motor to keep them spinning and to activate the stabilization.  

To do this:   
1. Decide on a switch you will use to activate Idle up
1. In the mixer menu add a line under Throttle and mix in a value of between 4 and 6 to be added to the throttle value if the switch is activated.  What this does is effectively prevents the throttle from 

#Inductrix (Horizon Hobby)

<img src="https://s7d5.scene7.com/is/image/horizonhobby/BLH8700_a0" Width="200" Height="200" />

##Binding
{Enter bind instructions here - Which DSM mode works best?}

##Tx Setup
{How to setup the transmitter optimally for leveling and acro mode}
