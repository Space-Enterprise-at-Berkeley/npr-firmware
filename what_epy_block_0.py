
import numpy as np
from gnuradio import gr
import time, socket
from collections import deque

ip = "127.0.0.1"
destip = "10.0.0.42"
port = 42069

testing_check = False

good_packets, bad_packets = 0, 0
prevTime = 0
packetDict = {}

def checkErrorRate(self):
    if (time.time() - self.prevTime) > 2:
        tP = (self.good_packets + self.bad_packets)
        if (tP > 0):
            erate = self.good_packets / tP
            print(f"% good in last 2 seconds: {erate*100}, total good packets: {self.good_packets}")
        else:
            print(f"% good in last 2 seconds: --")
 
        self.prevTime = time.time()
        self.good_packets, self.bad_packets = 0, 0

    

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


def computeChecksum(buff):
    sum1, sum2 = 0, 0

    id = buff[0]
    length = buff[1]
    timestamp = buff[2:6]
    checksum = buff[6:8]
    data = buff[8:]

    sum1 = sum1 + id
    sum2 = sum2 + sum1
    sum1, sum2 = (sum1%256), (sum2%256)


    sum1 = sum1 + length
    sum2 = sum2 + sum1
    sum1, sum2 = (sum1%256), (sum2%256)

    for i in range(4):
        sum1 = sum1 + timestamp[i]
        sum2 = sum2 + sum1
        sum1, sum2 = (sum1%256), (sum2%256)
    
    for i in range(length):
        sum1 = sum1 + data[i]
        sum2 = sum2 + sum1
        sum1, sum2 = (sum1%256), (sum2%256)

    return ((sum2 << 8)|sum1)

    



def split(self, buff):

    #*flight* packets are combined into one radio packet (max 128 bytes)
    #so this function splits them into flight packets
    ctr = 0
    try:
        while True:
            if (len(buff) < 9):
                return
            id = buff[0]
            length = buff[1]
            timestamp = (buff[2] << 24) + (buff[3] << 16) + (buff[4] << 8) + (buff[5])
            checksum = (buff[7] << 8) + buff[6]

            #print(f"id: {id}, length: {length}, timestamp: {timestamp}, checksum: {checksum}, expected checkum: {computeChecksum(buff)}")
            #print(f"buffer length {len(buff)}, expected length {length + 8}")
            if (computeChecksum(buff) == checksum):
                ctr += 1
                #print(f"sending packet #{ctr} with id: {id}, length: {length}")
                self.good_packets += 1
                try:
                    if (packetDict[id] != timestamp):
                        sendover(self, buff)
                        packetDict[id] = timestamp
                except KeyError:
                    packetDict[id] = timestamp
                

            else:
                # print("CHECKSUM ERROR")
                self.bad_packets += 1
            buff = buff[8+length:]
    except Exception as e:
        return


def testing_add(self, i):
    #print(i)
    self.lastPacketTime = time.time()
    check = True
    packet_start = i[0]
    if (68 < packet_start < 71):
        f = i[1]
        for j in range(1, i+[0]*100):
            if (i[j] != f+j-1):
                check = False
    else:
        check = False
    if check : self.testctr+=1
            

def sendRssiPacket(self, rssi):
    buff = []
    buff.append(56)
    buff.append(2)
    buff += [0]*6
    buff.append(int(rssi) % 256)
    buff.append(min(255, abs(int(rssi) >> 8)))
    i = computeChecksum(buff)
    buff[6] = min(255, abs(int(i % 256)))
    buff[7] = min(255, abs(int(i >> 8)))
#    print(buff)
    sendover(self, buff)
    
    
def sendover(self, buff):
    #sends a flight packet to the GS
    
    if testing_check:
        testing_add(self, buff)


    
    f = list(destip)
    f = [ord(i) for i in f]
    f = [len(f)] + f
    f += buff
    
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
            in_sig=[np.byte, np.float32],
            out_sig=[np.byte]
        )
        self.set_min_output_buffer(8192)
        self.cleanBuffer = ""
        self.fullBuffer = ""
        self.cumPackets = 0
        self.lastProcessTime = time.time()
        self.preval = 0
        self.cumctr = 0
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.testctr = 1
        self.lastPacketTime = 0
        self.good_packets, self.bad_packets = 0, 0
        self.prevTime = 0
        self.rssiDeque = deque(maxlen = 100000)
        self.rssi = 0
        self.prevRssiTime = time.time()

    def work(self, input_items, output_items):
#        print(f"hi with {len(input_items[0])} samples")

        checkErrorRate(self)

        # if (testing_check): checkPacketTime(self)
        # # t1 = (input_items[1][:] == 1)
        # # t2 = (input_items[0])[t1]

        t2 = input_items[0]
        
        # for i in input_items[1]: #the avg received power (not necessarily signal strength)
        #     self.rssiDeque.append(i)
            
        # fg = sorted(self.rssiDeque)
        # self.rssi = sum(fg[-300:]) / sum(fg[:300])
#        print(self.rssi)
#         if (time.time() - self.prevRssiTime > 0.25):
        
# #            sendRssiPacket(self, self.rssi)
#             self.prevRssiTime = time.time()
        
        
        # print(t2[:100])
        g = str(t2)
        bad = ['[', ']', '\n', ' ', ',']
        for i in bad:
            g = g.replace(i, "")

        #g is good str.
        self.fullBuffer += g
        
        if len(self.fullBuffer) > 100:
            try:
                (shortened, self.cumctr, self.preval) = clean(self.fullBuffer, self.cumctr, self.preval)
            except Exception as e:
                print("clean error")
                return len(output_items[0])
            self.fullBuffer = ""
            self.cleanBuffer += shortened
        
        
        if ((time.time() - self.lastProcessTime > 0.1) or len(self.cleanBuffer) > 100):
            #print(self.cleanBuffer)
            self.lastProcessTime = time.time()
            # print(f"processing {len(self.cleanBuffer)} bits")
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

