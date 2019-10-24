//  Author: Garrett Smith
//  NetID: gas203

//  Sources Consulted
//  1) https://www.reddit.com/r/learnprogramming/comments/3qotqr/how_can_i_read_an_entire_text_file_into_a_string/cwh1lf6/
//  2) https://www.geeksforgeeks.org/convert-string-char-array-cpp/

#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <fstream>

#include "packet.cpp"

#define MAX_BUFFER_SIZE 256
#define PACKET_DATA_LENGTH 30

using namespace std;

int main(int, char *argv[]) {
    struct hostent *s;
    struct sockaddr_in send_server;
    struct sockaddr_in recv_server;
    char file_buffer[MAX_BUFFER_SIZE];
    char send_buffer[MAX_BUFFER_SIZE];
    char recv_buffer[MAX_BUFFER_SIZE];
    int sockfd;
    
    const char *emulator_host = argv[1];
    int send_port;
    int recv_port;
    const char *filename = argv[4];
      
    char null_data[32];
    memset(null_data, 0, 32);
    
    istringstream(argv[2]) >> send_port;
    istringstream(argv[3]) >> recv_port;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    s = gethostbyname(emulator_host);
    memset((char *) &send_server, 0, sizeof(send_server));
    send_server.sin_family = AF_INET;
    send_server.sin_port = htons(send_port);
    bcopy((char *)s->h_addr, (char *)&send_server.sin_addr.s_addr, s->h_length);

    memset((char *) &recv_server, 0, sizeof(recv_server));
    recv_server.sin_family = AF_INET;
    recv_server.sin_port = htons(recv_port);
    recv_server.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&recv_server, sizeof(recv_server));
    socklen_t rslen = sizeof(recv_server);

    ifstream infile(filename);
    stringstream file_contents_stream;

    file_contents_stream << infile.rdbuf();
    string file_contents = file_contents_stream.str();
    int file_length = file_contents.length();

    for (int i = 0; i < file_length; i += PACKET_DATA_LENGTH) {
        string data = file_contents.substr(i, PACKET_DATA_LENGTH);
        int data_length = data.length();
        
        char data_array[data_length + 1];
        strcpy(data_array, data.c_str());

        packet pkt(1, 0, data_length, data_array);
        pkt.printContents();
        pkt.serialize(send_buffer);

        sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&send_server, sizeof(send_server));
        recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_server, &rslen);

        packet ack(0, 0, 30, null_data);
        ack.deserialize(recv_buffer);

        if (ack.getType() == 0) {
            cout << "ACK RECEIVED" << endl;
        } else {
            cout << "INVALID ACK" << endl;
            return 1;
        }
    }

    packet end_pkt(3, 0, 0, NULL);
    end_pkt.serialize(send_buffer);
    sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&send_server, sizeof(send_server));

    recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_server, &rslen);

    packet pkt(0, 0, 30, null_data);
    pkt.deserialize(recv_buffer);

    if(pkt.getType() == 2) {
        close(sockfd);
        return 0;
    } else {
        cout << "INVALID ENDPACKET" << endl;
        return 1;
    }

 
}
