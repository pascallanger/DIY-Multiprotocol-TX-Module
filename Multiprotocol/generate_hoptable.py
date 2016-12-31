#!/usr/bin/python
#
# this will generate a random frsky compatible
# hop table and a random txid
#
import random

random.seed()

#get a random number for the txid
txid = random.randint(513, 65000)

#get random numbers for the hoptable calculation
channel_start   = random.randint(0, 7) 
channel_spacing = random.randint(64, 255-64)

#generate hoptable
hoptable = []
hop = channel_start
for i in range(47):
	hoptable.append(hop)
	hop = (hop + channel_spacing) % 235
        if (hop == 0) or (hop == 0x5A) or (hop == 0xDC):
		hop = hop + 1

hoptable_s = (",".join("0x{:02X} ".format(val) for val in hoptable))

print("#ifndef __HOPTABLE_H__")
print("#define __HOPTABLE_H__")
print("")
print("#define FRSKY_DEFAULT_FSCAL_VALUE 0x00")
print("")
print("#define FRSYK_TXID (0x%04X)" % (txid))
print("")
print("//hoptable was generated with start=%d, spacing=%d" % (channel_start, channel_spacing))
print("#define FRSKY_HOPTABLE { %s }" % (hoptable_s))
print("")
print("#endif  // __HOPTABLE_H__")
