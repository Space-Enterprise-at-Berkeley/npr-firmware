import adi, os
import numpy as np
from gnuradio import gr
import zmq, time

def set_freq(i):
    s = "iio_attr -u ip:192.168.2.1 -c ad9361-phy RX_LO frequency " + str(i)
    os.system(s)
def set_manual_gain():
    s = "iio_attr -u ip:192.168.2.1 -c ad9361-phy voltage0 gain_control_mode manual"
    os.system(s)
def set_gain_level(i):
    s = "iio_attr -i -u ip:192.168.2.1 -c ad9361-phy voltage0 hardwaregain " + str(i)
    os.system(s)
def set_rf_bandwidth(i):
    s = "iio_attr -i -u ip:192.168.2.1 -c ad9361-phy voltage0 rf_bandwidth " + str(i)
    os.system(s)

class blk(gr.sync_block):


    def __init__(self):
        self.sdr = adi.Pluto('ip:192.168.2.1')
        self.sample_rate = int(1e6)
        self.center_freq = 450008000
        self.num_samps = int(10000)


        set_freq(self.center_freq)
        set_manual_gain()
        set_gain_level(30)
        set_rf_bandwidth(40000)
        
        self.sdr.rx_buffer_size = self.num_samps
#
        gr.sync_block.__init__(
            self,
            name='Embedded Python Block',
            in_sig=[np.float32],
            out_sig=[np.complex64]
        )
        self.starting = 1
    def work(self, input_items, output_items):
        print("called")
        if (self.starting == 1):
            self.sdr.rx_destroy_buffer()
            self.starting = 0;
        buf = self.sdr.rx()
        output_items[0] = buf
        print(len(buf))
        return len(output_items[0])
