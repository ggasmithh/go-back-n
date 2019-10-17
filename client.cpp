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
    struct sockaddr_in server;
    struct sockaddr_in response_server;
    const char *hostname = argv[1];
    int port;
    char transaction_end[MAX_BUFFER_SIZE] = "1234567";
    char file_buffer[MAX_BUFFER_SIZE];
    char send_buffer[MAX_BUFFER_SIZE];
    int sockfd;
        
    istringstream(argv[2]) >> port;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    s = gethostbyname(hostname);
    memset((char *) &server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    bcopy((char *)s->h_addr, (char *)&server.sin_addr.s_addr, s->h_length);

    memset((char *) &response_server, 0, sizeof(response_server));
    response_server.sin_family = AF_INET;
    response_server.sin_port = htons(port);
    response_server.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&response_server, sizeof(response_server));

    ifstream infile(argv[3]);
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

        sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&server, sizeof(server));
    }


    packet end_pkt(1, 0, MAX_BUFFER_SIZE, transaction_end);
    end_pkt.serialize(send_buffer);
    sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&server, sizeof(server));
    close(sockfd);

    return 0;
}
