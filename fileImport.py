import struct


class FileLoad:
    PACKET_SIZE_STATE = 796
    PACKET_SIZE_ACTION = 4
    PACKET_SIZE_TYPE = 4
    DEBUGMODE = False

    def __init__(self, path):
        if path == '':
            path = 'F:\work\cocos\dqnTest\Resources\scenario - Copy.sce'
        self.fp = open(path, 'rb')
        self.data = bytearray(self.fp.read())
        self.offset = 0        
    def close(self):
        self.fp.close()        
    def getType(self):
        buffer = self.data[self.offset:self.offset + self.PACKET_SIZE_TYPE]
        self.offset = self.offset + self.PACKET_SIZE_TYPE
        [t] = struct.unpack('i', buffer)
        return t
    def readState(self):        
        if self.DEBUGMODE == False:
            if self.hasMore() == False:
                return -1
            t = self.getType() 
            if t != 1:
                return -1, -1, -1, -1, -1

        buffer = self.data[self.offset:self.offset + self.PACKET_SIZE_STATE]
        self.offset = self.offset + self.PACKET_SIZE_STATE

        [id, reward, totalScore, isTerminal] = struct.unpack('iiii', buffer[:16])
        arr = []
        for i in range(3):
            idx = 16 + (i * 65 * 4)
            idx_e = idx + (65*4)
            arr.append(struct.unpack("65f", buffer[idx:idx_e]))
            
        if isTerminal == 0:
            isTerminal = False
        else :
            isTerminal = True

        return id, reward, totalScore, isTerminal, arr

    def readAction(self):        
        if self.DEBUGMODE == False:
            if self.hasMore() == False:
                return -1

            t = self.getType()
            if t != 2:
                return -1

        buffer = self.data[self.offset:self.offset + self.PACKET_SIZE_ACTION]
        self.offset = self.offset + self.PACKET_SIZE_ACTION
        [action] = struct.unpack('i', buffer)
        return action
    def hasMore(self):
        length = len(self.data)
        if length <= self.offset + 4:
            return False
        return True


DEBUGMODE = False     
if DEBUGMODE == True:
    fp = FileLoad('')
    while fp.hasMore():
        if fp.getType() == 1:
            [id, reward, totalScore, isTerminal, arr] = fp.readState()
            if id == 581:
                print('')
            print (id, reward, totalScore, isTerminal, arr)
        else:
            action = fp.readAction()
            print (action)

    fp.close()
