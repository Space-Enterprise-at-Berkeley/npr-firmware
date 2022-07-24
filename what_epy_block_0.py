
import numpy as np
from gnuradio import gr
import time, socket

ip = "127.0.0.1"
destip = "10.0.0.42"
port = 42069

testing_check = True

def clean(i, cumctr, preval):
    
    #decimates by a factor of 25-ish to eliminated repeated sampling
    #should've just sampled slower ¯\_(ツ)_/¯
    
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
        if val == '': continue
        if (val != preval):
            #print(cumctr/25)
            out += [preval]*int(round(cumctr/25))
            cumctr = 0
        cumctr += 1
        ctr += 1
        preval = val
    return ("".join(out), cumctr, preval)
    
def split(self, i):

    #*flight* packets are combined into one radio packet (max 128 bytes)
    #so this function splits them into flight packets
    
    #print(i)
    sendover(self, i)
#    packets = []
#    while (len(i) > 1):
#        length = i[1]
#        end = 8 + length #id, len, time(4), checksum(2)
#        packets.append(i[:end])
#        i = i[end:]
#    for i in packets:
#        print(i)
#        sendover(self, i)

def testing_add(self, i):
    self.lastPacketTime = time.time()
    check = True
    packet_start = i[0]
    if (68 < packet_start < 71):
        f = i[1]
        for j in range(1, 100):
            if (i[j] != f+j-1):
                check = False
    else:
        check = False
    if check : self.testctr+=1
            

def sendover(self, i):
    #sends a flight packet to the GS
    
    if testing_check:
        testing_add(self, i)
    
    f = list(destip)
    f = [ord(i) for i in f]
    f = [len(f)] + f
    f += i
    self.sock.sendto(bytes(f), (ip, port))
    
def checkPacketTime(self):
    if (self.testctr == 0): return
    if (time.time() - self.lastPacketTime) > 3:
        print(f"last packet {(time.time() - self.lastPacketTime):.1f}s ago: got {self.testctr}/200 packets -> {(self.testctr/2):.2f}%\n")
        self.testctr = 0
    
    

def parse(self, i, pn):
    
    #converts binary data to radio packets, removing noise from either side
    
    i = "".join([str(j) for j in i])
    try:
        j = i.index("0010110111010100")
    except:
        print("packet not found")
        return 0
    i = i[j+16:]
    try:
        length = int(i[:8], 2)
    except Exception as e:
        print(f"#1 - {i[:8]}")
    #print(f"packet #{pn+1}")
    #print(f"found packet #{pn+1} with length {length}", end ="\n")
    
    i=i[24:]
    #print("0x", end=" ")
    out = []
    for j in range(length):
        try:
            s = int(i[:8],2)
            out.append(s)
        except Exception as e:
            print("discarded packet")
            return 0
        i=i[8:]
    #print(out)
    split(self, out)
    return 1
   

    
   
class blk(gr.sync_block):
    def __init__(self):
        np.set_printoptions(threshold=np.inf)
        gr.sync_block.__init__(
            self,
            name='yeet',
            in_sig=[np.byte, np.byte],
            out_sig=[np.byte]
        )
        self.cleanBuffer = ""
        self.fullBuffer = ""
        self.cumPackets = 0
        self.lastProcessTime = time.time()
        self.preval = 0
        self.cumctr = 0
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.testctr = 1
        self.lastPacketTime = 0

    def work(self, input_items, output_items):
#        print(f"hi with {len(input_items[0])} samples")
        
        if (testing_check): checkPacketTime(self)
        t1 = (input_items[1][:] == 1)
        t2 = (input_items[0])[t1]
        
        g = str(t2)
        bad = ['[', ']', '\n', ' ', ',']
        for i in bad:
            g = g.replace(i, "")

        #g is good str.
        self.fullBuffer += g
        
        if len(self.fullBuffer) > 1000:
            try:
                (shortened, self.cumctr, self.preval) = clean(self.fullBuffer, self.cumctr, self.preval)
            except Exception as e:
                print("clean error")
                return len(output_items[0])
            self.fullBuffer = ""
            self.cleanBuffer += shortened
        
        
        if ((time.time() - self.lastProcessTime > 0.001) and len(self.cleanBuffer) > 300):
            #print(self.cleanBuffer)
            self.lastProcessTime = time.time()
            #print(f"processing {len(self.cleanBuffer)} bits")
            s = self.cleanBuffer
            seq = "1010101010100010110111010100"
            res = [i for i in range(len(s)) if s.startswith(seq, i)]
            if (res):
                for i in range(len(res)-1):
                    packet = self.cleanBuffer[res[i]:res[i+1]]
                    numparsed = parse(self, packet, self.cumPackets)
                    #print(f"got {numparsed} packets, buffer has size {len(self.cleanBuffer)}")
                    if (numparsed == 0):
                        print(f"parse error - discarded buffer with size {len(self.cleanBuffer)}")
                        self.cleanBuffer = "0"
                        return len(output_items[0])
                    else:
                        self.cumPackets += numparsed
                self.cleanBuffer = self.cleanBuffer[res[-1]:]
                #print(self.cleanBuffer)
        #print(len(output_items[0]))

        
        #output_items[0][:] = (input_items[0])[temp]
        
        #output_items[0][:] = input_items[0] * self.example_param
        #return len(output_items[0])

        return len(output_items[0])

