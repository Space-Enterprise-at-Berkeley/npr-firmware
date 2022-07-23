
import numpy as np
from gnuradio import gr
import time

def clean(i, cumctr, preval):
    #f = open("/Users/sekharm/wtf.txt", "a")
    #print(i, end = "", file=f)
    #f.close()
    if len(i) == 0: return
    out = []
    ctr = 1
    cumctr = cumctr
    preval = preval
    while (ctr < len(i)):
        val = i[ctr]
        if (val != preval):
            #print(cumctr/25)
            out += [preval]*int(round(cumctr/25))
            cumctr = 0
        cumctr += 1
        ctr += 1
        preval = val
    return ("".join(out), cumctr, preval)
def parse(i, pn):
    i = "".join([str(j) for j in i])
    try:
        j = i.index("0010110111010100")
    except:
        print("packet not found")
        return 0
    i = i[j+16:]
    length = int(i[:8], 2)
    print(f"found packet #{pn+1} with length {length}:", end =" ")
    
    i=i[24:]
    print("0x", end=" ")
    for j in range(length):
        print(("0"+hex(int(i[:8],2))[2:])[-2:], end=" ")
        i=i[8:]
    print("")
    return 1
   
class blk(gr.sync_block):  # other base classes are basic_block, decim_block, interp_block

    def __init__(self, output_file_name="/dev/null"):
        np.set_printoptions(threshold=np.inf)
        gr.sync_block.__init__(
            self,
            name='yeet',   # will show up in GRC
            in_sig=[np.byte, np.byte],
            out_sig=[np.byte]
        )
        # if an attribute with the same name as a parameter is found,
        # a callback is registered (properties work, too).
        self.output_file_name = output_file_name
        self.cleanBuffer = ""
        self.fullBuffer = ""
        self.cumPackets = 0
        self.lastProcessTime = time.time()
        self.preval = 0
        self.cumctr = 0
        #f = open(self.output_file_name, "w")
        #f.close()

    def work(self, input_items, output_items):
#        print(f"hi with {len(input_items[0])} samples")
        
        t1 = (input_items[1][:] == 1)
        t2 = (input_items[0])[t1]
        
        g = str(t2)
        bad = ['[', ']', '\n', ' ', ',']
        for i in bad:
            g = g.replace(i, "")

        #g is good str.
        self.fullBuffer += g
        
        if len(self.fullBuffer) > 1000:
            (shortened, self.cumctr, self.preval) = clean(self.fullBuffer, self.cumctr, self.preval)
            self.fullBuffer = ""
            self.cleanBuffer += shortened
        
        
        if ((time.time() - self.lastProcessTime > 0.01) and len(self.cleanBuffer) > 600):
            #print(self.cleanBuffer)
            self.lastProcessTime = time.time()
            #print(f"processing {len(self.cleanBuffer)} bits")
            s = self.cleanBuffer
            seq = "1010100010110111010100"
            res = [i for i in range(len(s)) if s.startswith(seq, i)]
            if (res):
                for i in range(len(res)-1):
                    packet = self.cleanBuffer[res[i]:res[i+1]]
                    self.cumPackets += parse(packet, self.cumPackets)
                self.cleanBuffer = self.cleanBuffer[res[-1]:]
                #print(self.cleanBuffer)
        #print(len(output_items[0]))

        
        #output_items[0][:] = (input_items[0])[temp]
        
        #output_items[0][:] = input_items[0] * self.example_param
        #return len(output_items[0])

        return len(output_items[0])

