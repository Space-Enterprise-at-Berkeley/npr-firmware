#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Not titled yet
# GNU Radio version: 3.10.3.0

from gnuradio import analog
import math
from gnuradio import blocks
from gnuradio import digital
from gnuradio import filter
from gnuradio.filter import firdes
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import soapy
import what_epy_block_0 as epy_block_0  # embedded python block




class what(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "Not titled yet", catch_exceptions=True)

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 300000
        self.update_rate = update_rate = 0.00001
        self.num_pts = num_pts = int(samp_rate * 0.2)
        self.fsk_deviation_hz = fsk_deviation_hz = 20000
        self.fname = fname = "firstrealpls"

        ##################################################
        # Blocks
        ##################################################
        self.soapy_rtlsdr_source_0 = None
        dev = 'driver=rtlsdr'
        stream_args = ''
        tune_args = ['']
        settings = ['']
        try:
            self.soapy_rtlsdr_source_0 = soapy.source(dev, "fc32", 1, '',
                                  stream_args, tune_args, settings)
        except RuntimeError as e:
            print(f"plug in your sdr! [{str(e)}]")
            quit()
        self.soapy_rtlsdr_source_0.set_sample_rate(0, samp_rate)
        self.soapy_rtlsdr_source_0.set_gain_mode(0, False)
        self.soapy_rtlsdr_source_0.set_frequency(0, 450003000)
        self.soapy_rtlsdr_source_0.set_frequency_correction(0, 0)
        self.soapy_rtlsdr_source_0.set_gain(0, 'TUNER', 30)
        self.freq_xlating_fir_filter_xxx_0 = filter.freq_xlating_fir_filter_ccc(1, firdes.low_pass(1.0,samp_rate, 37500, 5000), 0, samp_rate)
        self.epy_block_0 = epy_block_0.blk()
        self.digital_binary_slicer_fb_0 = digital.binary_slicer_fb()
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_char*1)
        self.blocks_multiply_const_xx_0 = blocks.multiply_const_ff(100, 1)
        self.blocks_moving_average_xx_0 = blocks.moving_average_ff(10000, 1, 10000, 1)
        self.blocks_complex_to_mag_squared_0 = blocks.complex_to_mag_squared(1)
        self.analog_quadrature_demod_cf_0 = analog.quadrature_demod_cf((3*(samp_rate/(2*math.pi*fsk_deviation_hz))))


        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_quadrature_demod_cf_0, 0), (self.digital_binary_slicer_fb_0, 0))
        self.connect((self.blocks_complex_to_mag_squared_0, 0), (self.blocks_multiply_const_xx_0, 0))
        self.connect((self.blocks_moving_average_xx_0, 0), (self.epy_block_0, 1))
        self.connect((self.blocks_multiply_const_xx_0, 0), (self.blocks_moving_average_xx_0, 0))
        self.connect((self.digital_binary_slicer_fb_0, 0), (self.epy_block_0, 0))
        self.connect((self.epy_block_0, 0), (self.blocks_null_sink_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.analog_quadrature_demod_cf_0, 0))
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.blocks_complex_to_mag_squared_0, 0))
        self.connect((self.soapy_rtlsdr_source_0, 0), (self.freq_xlating_fir_filter_xxx_0, 0))


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_num_pts(int(self.samp_rate * 0.2))
        self.analog_quadrature_demod_cf_0.set_gain((3*(self.samp_rate/(2*math.pi*self.fsk_deviation_hz))))
        self.freq_xlating_fir_filter_xxx_0.set_taps(firdes.low_pass(1.0,self.samp_rate, 37500, 5000))
        self.soapy_rtlsdr_source_0.set_sample_rate(0, self.samp_rate)

    def get_update_rate(self):
        return self.update_rate

    def set_update_rate(self, update_rate):
        self.update_rate = update_rate

    def get_num_pts(self):
        return self.num_pts

    def set_num_pts(self, num_pts):
        self.num_pts = num_pts

    def get_fsk_deviation_hz(self):
        return self.fsk_deviation_hz

    def set_fsk_deviation_hz(self, fsk_deviation_hz):
        self.fsk_deviation_hz = fsk_deviation_hz
        self.analog_quadrature_demod_cf_0.set_gain((3*(self.samp_rate/(2*math.pi*self.fsk_deviation_hz))))

    def get_fname(self):
        return self.fname

    def set_fname(self, fname):
        self.fname = fname




def main(top_block_cls=what, options=None):
    tb = top_block_cls()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        sys.exit(0)

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    tb.start()

    tb.wait()


if __name__ == '__main__':
    main()
