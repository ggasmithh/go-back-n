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

int file_writer(string data) {
    // Write file
    ofstream output("dataReceived.txt", ios_base::trunc);
    output << data;
    output.close();

    return 0;
}


int main(int, char* argv[]) {
    struct sockaddr_in server;
    struct sockaddr_in client;
    int port;
    const char *transaction_end = "1234567";
    char recv_buffer[MAX_BUFFER_SIZE];
    char file_buffer[MAX_BUFFER_SIZE];
    string complete_data = "";
    int sockfd;
    
    istringstream(argv[1]) >> port;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 
    memset((char *)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
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

        string data = pkt.getData();

        char compare_data[data.length() + 1];
        strcpy(compare_data, data.c_str());

        if(strcmp(compare_data, transaction_end) == 0) {
            break;
        } else {
            complete_data += data;
        }
    } 
    
    close(sockfd);

    file_writer(complete_data);

    // // Write file
    // ofstream output("dataReceived.txt", ios_base::trunc);
    // output << complete_data;
    // output.close();

    return 0;
}
