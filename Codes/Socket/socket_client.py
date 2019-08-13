# a reconnect socket client.
# the main function is to receive data from server and print it.
# manage stream of characters by detecting eol.

import socket
from time import sleep
#from subprocess import check_output

buff = []

def convert(s): 

	# initialization of string to "" 
	new = "" 

	# traverse in the string 
	for x in s: 
		new += x 

	# return string 
	return new 
	

def Main():
        # 127.0.0.1 = local machine.
        # change to the address of the server accordingly
        host = "192.168.1.1"
        # port should be >1024
        port = 5000
         
        mySocket = socket.socket()
        connected = False

         
        while True:
                if(not connected):
                        try:
                                mySocket.connect((host,port))
                                connected = True
                                sleep(10)
                        except:
                                pass
                else:
                        try:
                                data = mySocket.recv(1).decode()
                                if not data:
                                       break
                                if data != '!':
                                       buff.append(data)
                                else:
                                       buff.append(data)
                                       print ('Received from server: ' + convert(buff))
                                       #print ('Received from channel' + channels.index(current_freq) + ' :' + convert(buff))
                                       buff.clear()
                                #print ('Received from server: ' + convert(data))

                        except:
                                mySocket = socket.socket()
                                connected = False
                                pass
                        #time.sleep(1)
                #data = str(data).upper()
                #print ("echo: " + str(data))
                #mySocket.send(data.encode())
                 
        mySocket.close()

while(1): 
#if __name__ == '__main__':
    Main()
    sleep(10)
