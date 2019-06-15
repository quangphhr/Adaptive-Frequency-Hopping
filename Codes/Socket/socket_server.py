# a simple socket server
# remove comment line to receive reply from client
import socket
from time import sleep

    # accept any address 
host = ""
    # port should be >1024
port = 5000
     
mySocket = socket.socket()
mySocket.bind((host,port))


def Online():
     
    mySocket.listen(1)
    conn, addr = mySocket.accept()
    print ("Connection from: " + str(addr))

    message = input(" -> ")
    try:
        while True:
            conn.send(message.encode())
            #data = conn.recv(1024).decode()
            #print ("from connected  user: " + str(data))
            
            message = input(" -> ")
            
    except (SystemExit, socket.error):
        conn.close()
             
    conn.close()
     
while True:
    Online()
    sleep(1)
