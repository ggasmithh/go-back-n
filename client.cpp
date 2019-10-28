//  Author: Garrett Smith, Tom Jackson
//  NetID: gas203, 

//  Sources Consulted
//  1) First programming assignment
//  2) https://en.wikipedia.org/wiki/Go-Back-N_ARQ
//  3) https://stackoverflow.com/questions/21542077/c-sigalrm-alarm-to-display-message-every-second
//  4) https://linux.die.net/man/3/alarm
//  5) https://stackoverflow.com/questions/2876024/linux-is-there-a-read-or-recv-from-socket-with-timeout


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
#include <vector>
#include <signal.h>

#include "packet.cpp"

#define MAX_BUFFER_SIZE 256
#define MAX_PACKET_DATA_LENGTH 30
#define N 7

using namespace std;

volatile sig_atomic_t timeout = false;

void handle_alarm(int sig) {
    timeout = true;
}

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

    signal(SIGALRM, handle_alarm);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    ofstream client_seq_num("clientseqnum.log", ios_base::trunc);
    ofstream client_ack("clientack.log", ios_base::trunc);

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

    int next_seq_num = 0;
    int send_base = 0;
    int seq_max = N + 1;    // this should be 8

    int actual_data_length = MAX_PACKET_DATA_LENGTH;
    int file_cur_loc = 0;
    int file_loc_offset = 0;
    char data_array[MAX_PACKET_DATA_LENGTH];

    // We're adding a whole packet of data's worth here, because the very last chunk
    // May not be a full 30 characters, so we want the loop to continue
    while (file_cur_loc + actual_data_length < file_length) {

        memset(data_array, 0, MAX_PACKET_DATA_LENGTH);

        if (next_seq_num < send_base + N) {

            file_cur_loc = (next_seq_num * MAX_PACKET_DATA_LENGTH) + file_loc_offset;
            actual_data_length = min(MAX_PACKET_DATA_LENGTH, file_length - file_cur_loc);
        
            string data = file_contents.substr(file_cur_loc, actual_data_length);
            strcpy(data_array, data.c_str());

            packet pkt(1, next_seq_num, actual_data_length, data_array);
            pkt.printContents();
            pkt.serialize(send_buffer);
            
            sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&send_server, sizeof(send_server));

            client_seq_num << next_seq_num << endl;

            if (next_seq_num == send_base) {
                alarm(2);
            }

            next_seq_num = (next_seq_num + 1) % seq_max;
            if (next_seq_num == 0) {
                file_loc_offset += min((MAX_PACKET_DATA_LENGTH * seq_max), file_length - 30);
            }
        }

        int timeout_check = 0;
        timeout_check = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_server, &rslen);
        if (timeout) {
            alarm(0);
            
            bind(sockfd, (struct sockaddr *)&recv_server, sizeof(recv_server));

            while(send_base != next_seq_num) {
                int timeout_file_cur_loc = (next_seq_num * MAX_PACKET_DATA_LENGTH) + file_loc_offset;
                int timeout_actual_data_length = min(MAX_PACKET_DATA_LENGTH, file_length - timeout_file_cur_loc);
                string timeout_data = file_contents.substr(timeout_file_cur_loc, timeout_actual_data_length);

                memset(data_array, 0, MAX_PACKET_DATA_LENGTH);
                strcpy(data_array, timeout_data.c_str());

                packet pkt(1, send_base, MAX_PACKET_DATA_LENGTH, data_array);
                pkt.printContents();
                pkt.serialize(send_buffer);

                timeout_check = -1;

                while (timeout_check < 0 ) {
                    sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&send_server, sizeof(send_server));
                    timeout_check = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_server, &rslen);
                }

                packet timeout_ack(0, 0, 30, null_data);
                timeout_ack.deserialize(recv_buffer);

                client_seq_num << next_seq_num << endl;
                client_ack << timeout_ack.getSeqNum() << endl;

                send_base = (send_base + 1) % seq_max;

            }
        }


        if (!timeout) {
            packet ack(0, 0, 30, null_data);
            ack.deserialize(recv_buffer);

            client_ack << ack.getSeqNum() << endl;

            send_base = (send_base + ack.getSeqNum()) % seq_max;       
        } else {
            timeout = false;
        }
         

        if (send_base == next_seq_num) {
            alarm(0);
            timeout = false;
        } else {
            alarm(2);
        }

    }

    packet end_pkt(3, 0, 0, null_data);
    end_pkt.serialize(send_buffer);

    sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&send_server, sizeof(send_server));
    client_seq_num << "0" << endl;

    recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_server, &rslen);
    client_ack << "0";

    packet pkt(0, 0, 30, null_data);
    pkt.deserialize(recv_buffer);

    if(pkt.getType() == 2) {
        close(sockfd);
        client_seq_num.close();
        client_ack.close();
        return 0;
    } else {
        return 1;
    }

 
}
