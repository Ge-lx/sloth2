#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>

class UPDSocket {

private:
    int sockfd;
    struct sockaddr_in client_addr;

public:
    UDPSocket (uint16_t client_port, char* client_address) {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0 ) {
            throw std::invalid_argument("Could not create socket file descriptor.");
        }

        memset(&client_addr, 0, sizeof(client_addr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_port = htons(client_port);
        inet_aton(client_address, &myaddr.sin_addr.s_addr);
    }

    ~UDPSocket () {
        close(sockfd);
    }

    void send_message (const void* data, size_t len) {
        sendto(sockfd, data, len, MSG_CONFIRM, (const struct sockaddr *) &client_addr, sizeof(client_addr));
    }
}