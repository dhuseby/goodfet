#!/usr/bin/env python

import sys;
import binascii;

from GoodFET import GoodFET;
from intelhex import IntelHex16bit;



if(len(sys.argv)==1):
    print "Usage: %s verb [objects]\n" % sys.argv[0];
    print "%s test" % sys.argv[0];
    print "%s dump $foo.hex [0x$start 0x$stop]" % sys.argv[0];
    print "%s erase" % sys.argv[0];
    print "%s flash $foo.hex [0x$start 0x$stop]" % sys.argv[0];
    print "%s verify $foo.hex [0x$start 0x$stop]" % sys.argv[0];
    sys.exit();

#Initailize FET and set baud rate
client=GoodFET();
client.serInit("/dev/ttyUSB0")



#Connect to target
client.CCsetup();
client.CCstart();

if(sys.argv[1]=="test"):
    client.CCtest();
if(sys.argv[1]=="dump"):
    f = sys.argv[2];
    start=0x0200;
    stop=0xFFFF;
    if(len(sys.argv)>3):
        start=int(sys.argv[3],16);
    if(len(sys.argv)>4):
        stop=int(sys.argv[4],16);
    
    print "Dumping from %04x to %04x as %s." % (start,stop,f);
    h = IntelHex16bit(None);
    i=start;
    while i<stop:
        h[i>>1]=client.MSP430peek(i);
        if(i%0x100==0):
            print "Dumped %04x."%i;
        i+=2;
    h.write_hex_file(f);
if(sys.argv[1]=="erase"):
  print "Status: %s" % client.CCstatusstr();
  client.CCchiperase();
  print "Status: %s" %client.CCstatusstr();

if(sys.argv[1]=="flash"):
    f=sys.argv[2];
    start=0;
    stop=0xFFFF;
    if(len(sys.argv)>3):
        start=int(sys.argv[3],16);
    if(len(sys.argv)>4):
        stop=int(sys.argv[4],16);
    
    h = IntelHex16bit(f);
    
    client.MSP430masserase();
    for i in h._buf.keys():
        #print "%04x: %04x"%(i,h[i>>1]);
        if(i>=start and i<stop  and i&1==0):
            client.MSP430writeflash(i,h[i>>1]);
            if(i%0x100==0):
                print "%04x" % i;
if(sys.argv[1]=="flashtest"):
    client.MSP430flashtest();
if(sys.argv[1]=="verify"):
    f=sys.argv[2];
    start=0;
    stop=0xFFFF;
    if(len(sys.argv)>3):
        start=int(sys.argv[3],16);
    if(len(sys.argv)>4):
        stop=int(sys.argv[4],16);
    
    h = IntelHex16bit(f);
    for i in h._buf.keys():
        if(i>=start and i<stop and i&1==0):
            peek=client.MSP430peek(i)
            if(h[i>>1]!=peek):
                print "ERROR at %04x, found %04x not %04x"%(i,peek,h[i>>1]);
            if(i%0x100==0):
                print "%04x" % i;

client.CCstop();