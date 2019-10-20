//  Author: Garrett Smith
//  NetID: gas203

//  Sources Consulted

#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>
#include <time.h>
#include <stdio.h>
#include <fstream>
#include <cctype>

#include "packet.cpp"

#define MAX_BUFFER_SIZE 256

using namespace std;

int main(int, char* argv[]) {
    struct sockaddr_in server;
    struct sockaddr_in client;
    char recv_buffer[MAX_BUFFER_SIZE];
    char send_buffer[MAX_BUFFER_SIZE];
    char file_buffer[MAX_BUFFER_SIZE];
    string complete_data = "";
    int sockfd;
    
    const char *emulator_host = argv[1];
    int recv_port;
    int send_port;
    const char *filename = argv[4];
    
    istringstream(argv[2]) >> recv_port;
    istringstream(argv[3]) >> send_port;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 
    memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(recv_port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&server, sizeof(server));
    
    socklen_t clen = sizeof(client);

    while (1) {
        memset((char *)&recv_buffer, 0, sizeof(recv_buffer));
        recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&client, &clen);
    
        char null_data[32];
        memset(null_data, 0, 32);

        packet pkt(0, 0, 30, null_data);
        pkt.deserialize(recv_buffer);

        if(pkt.getType() == 3) {
            packet end_pkt(2, 0, 0, NULL);
            end_pkt.serialize(send_buffer);
            server.sin_port = htons(send_port);
            sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&server, sizeof(server));
            break;
        } else {
            complete_data += pkt.getData();
        }
    } 
    
    close(sockfd);

    ofstream output(filename, ios_base::trunc);
    output << complete_data;
    output.close();

    return 0;
}

