all: client server

client: client.o
	g++ client.cpp -g -lstdc++ -o client
	
server: server.o
	g++ server.cpp -g -lstdc++ -o server
	
clean:
	\rm *.o client server
