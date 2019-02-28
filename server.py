import socket
import struct
import random
import sys

class Server:
    def __init__(self, host, port):
        self.HOST = host                  # Symbolic name meaning all available interfaces
        self.PORT = port              # Arbitrary non-privileged port

    def accept(self):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.bind((self.HOST, self.PORT))
        self.s.listen(1)
        self.conn, addr = self.s.accept()
        print('Connected by', addr)
    
    def readAction(self):
        data = self.conn.recv(8)
        [id, action] = struct.unpack('ii', data[:8])
        return id, action

    def readStatus(self):
        #print("recv start")
        data = self.conn.recv(796)
        #print("recv end")
        if not data:
            self.accept()
            return -1, -1, -1, -1, -1
        [id, reward, totalScore, isTerminal] = struct.unpack('iiii', data[:16])
        arr = []
        for i in range(3):
            idx = 16 + (i * 65 * 4)
            idx_e = idx + (65*4)
            arr.append(struct.unpack("65f", data[idx:idx_e]))
            
        if isTerminal == 0:
            isTerminal = False
        else :
            isTerminal = True
            #print("id", id, "totalScore", totalScore)

        buf = struct.pack('iii', id, id, id)
        self.conn.sendall(buf)
        
        
        return id, reward, totalScore, isTerminal, arr

    def sendX(self, id, x):
        buf = struct.pack('ii', id, x)
        self.conn.sendall(buf)
        #print("send")

    def close(self):
        self.conn.close()

        
        #if not data: break
        #conn.sendall(data)
    def test(self):
        self.HOST = ''                  # Symbolic name meaning all available interfaces
        self.PORT = 50007
        self.accept()
        
        while True:
            id, reward, totalScore, isTerminal, arr = self.readStatus()
            x = random.uniform(-1, 1)
            print(id, reward, totalScore, isTerminal, arr)
            print("x:", x)
            self.sendX(id, x)

''' 
server = Server('', 50007)
server.test()
'''