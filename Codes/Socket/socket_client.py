# a simple socket client
# the main function is to receive data from server and print it.
# can make it an echo client by removing comment line
import socket
buff = []
# Python program to convert a list 
# of character 

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
        mySocket.connect((host,port))
         
        while True:
                data = mySocket.recv(1).decode()
                if not data:
                        break
                if data != '!':
                        buff.append(data)
                elif data == '!' :
                        buff.append(data)
                        print ('Received from server: ' + convert(buff))
                        buff.clear()
                #data = str(data).upper()
                #print ("echo: " + str(data))
                #mySocket.send(data.encode())
                 
        mySocket.close()
 
if __name__ == '__main__':
    Main()