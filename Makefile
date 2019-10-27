all: client server

client: client.o
	g++ client.cpp -lstdc++ -o client
	
server: server.o
	g++ server.cpp -lstdc++ -o server
	
clean:
	\rm *.o client server
