# a simple socket client
# the main function is to receive data from server and print it.
# can make it an echo client by removing comment line
import socket
 
def Main():
            # 127.0.0.1 = local machine.
            # change to the address of the server accordingly
        host = "192.168.1.1"
            # port should be >1024
        port = 5000
         
        mySocket = socket.socket()
        mySocket.connect((host,port))
         
        while True:
                data = mySocket.recv(1024).decode()
                if not data:
                        break
                print ('Received from server: ' + str(data))
                 
                #data = str(data).upper()
                #print ("echo: " + str(data))
                #mySocket.send(data.encode())
                 
        mySocket.close()
 
if __name__ == '__main__':
    Main()
