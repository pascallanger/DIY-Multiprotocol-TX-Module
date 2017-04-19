# Getting your Bind timing right. 
On many consumer models it it important for the Tx to send a bind signal in a narrow window once the model has powered up.  

If the bind signal is not recieved during this window, the bind sequence times out.  Try this:  

 1. power the transmitter up with the throttle stick high.  This will trigger the warning window on the transmitter and put a hold on the transmitter bind process.  
 1. turn on the model
 1. while holding the bind button (if in PPM mode), at the right moment bring the throttle down to instantly bring the transmitter into bind mode.  

If you are using Serial Mode it is best to check the Autobind box in the Model Settings menu.  This will automatically initiate a bind sequence as soon as the Tx module powers up (Note: the Tx module only powers up when the transmitter passes the Switch/Throttle Warning page).
