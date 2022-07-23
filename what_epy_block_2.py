"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import numpy as np
from gnuradio import gr
import adi, os, random

def setinits():
    commands = [
                "iio_attr -i -u ip:192.168.2.1 -c ad9361-phy voltage0 sampling_frequency 3000000"
                "iio_attr -i -u ip:192.168.2.1 -c ad9361-phy voltage0 gain_control_mode manual"
                "iio_attr -i -u ip:192.168.2.1 -c ad9361-phy voltage0 hardwaregain 71"
                "iio_attr -u ip:192.168.2.1 -c ad9361-phy altvoltage0 frequency 450008000"
    ]
    print("hii")
    for i in commands:
        os.system(i)
    


    
    

class blk(gr.sync_block):  # other base classes are basic_block, decim_block, interp_block
    """Embedded Python Block example - a simple multiply const"""

    def __init__(self):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        gr.sync_block.__init__(
            self,
            name='ayo',   # will show up in GRC
            in_sig=[np.complex64],
            out_sig=[np.complex64]
        )
        self.ctr = 0
        self.sdr = adi.Pluto('ip:192.168.2.1')
        self.sdr.rx_buffer_size = 50000
        self.sdr.rx_destroy_buffer()
        self.starting = 1
        
        self.startptr = 0
        self.endptr = 0
        self.jankbuff = np.zeros(1000000) + 0.0j
        # if an attribute with the same name as a parameter is found,
        # a callback is registered (properties work, too).
    def refillbuffer(self):
        start, end = self.startptr, self.endptr
        
        f = self.sdr.rx() / (2**14)
        self.jankbuff[:end-start] = self.jankbuff[start:end]
        self.jankbuff[end-start:end-start+50000][:] = f
        
        #print(f"filledbuffer: start: {start}, end: {end}, newend: {end-start+4096}")
        print("refilled buffer" + str(random.randint(0, 9)))
        self.endptr = end-start+50000
        self.startptr = 0
        
    def work(self, input_items, output_items):
        if self.starting==1:
            setinits()
            self.starting = 0
        
        
        #print("called")
        
        #print(f"sdrlen {len(recv)}, output_items[0]len {len(output_items[0])}, input_items[0] len {len(input_items[0])}")
        #print(len(output_items[0]))
        
        l = len(input_items[0])
        
        if (self.endptr - self.startptr) < l:
            self.refillbuffer()
            
        output_items[0][:] = self.jankbuff[self.startptr:self.startptr+l]
        self.startptr = self.startptr + l
        #print(output_items[0][:50])
        
        return len(output_items[0])
        
        

