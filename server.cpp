//  Author: Garrett Smith, Tom Jackson
//  NetID: gas203, tpj24

//  Sources Consulted
//  1) First programming assignment
//  1) https://en.wikipedia.org/wiki/Go-Back-N_ARQ
//

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
#define N 7

using namespace std;

int main(int, char* argv[]) {
    // There *has* to be a better naming convention for these two sockaddr_in structs
    struct sockaddr_in recv_server;
    struct sockaddr_in send_server;
    char recv_buffer[MAX_BUFFER_SIZE];
    char send_buffer[MAX_BUFFER_SIZE];
    char file_buffer[MAX_BUFFER_SIZE];
    string complete_data = "";
    int sockfd;
    
    const char *emulator_host = argv[1];
    int recv_port;
    int send_port;
    const char *filename = argv[4];
 
    char null_data[32];
    memset(null_data, 0, 32);
    
    istringstream(argv[2]) >> recv_port;
    istringstream(argv[3]) >> send_port;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    ofstream arrival("arrival.log", ios_base::trunc);
 
    memset((char *)&recv_server, 0, sizeof(recv_server));
    recv_server.sin_family = AF_INET;
    recv_server.sin_port = htons(recv_port);
    recv_server.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&recv_server, sizeof(recv_server));
    socklen_t rslen = sizeof(recv_server);

    memset((char *)&send_server, 0, sizeof(send_server));
    send_server.sin_family = AF_INET;
    send_server.sin_port = htons(send_port);
    send_server.sin_addr.s_addr = htonl(INADDR_ANY); 

    int expected_seq_num = 0;

    while (1) {

        memset((char *)&recv_buffer, 0, sizeof(recv_buffer));
        
        recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_server, &rslen);             

        packet pkt(0, 0, 30, null_data);
        pkt.deserialize(recv_buffer);

        arrival << pkt.getSeqNum() << endl;

        if (pkt.getType() == 3) {
            packet end_pkt(2, 0, 0, NULL);
            end_pkt.serialize(send_buffer);
            sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&send_server, sizeof(send_server));
            break;
        } else if (pkt.getSeqNum() == expected_seq_num) {

            packet ack(0, expected_seq_num, 0, NULL);
            ack.serialize(send_buffer);
            sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&send_server, sizeof(send_server));

            expected_seq_num = (expected_seq_num + 1) % (N + 1);

            complete_data += pkt.getData();     
        } else {
            // whoof i'm really repeating myself here...
            packet ack(0, expected_seq_num, 0, NULL);
            ack.serialize(send_buffer);
            sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&send_server, sizeof(send_server));
        }
    } 
    
    close(sockfd);

    ofstream output(filename, ios_base::trunc);
    output << complete_data;
    output.close();

    return 0;
}

