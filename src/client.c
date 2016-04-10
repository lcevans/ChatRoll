#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "safeio.h"


int main(int arc, char **argv)
{
    printf("Welcome to Client v0.1!\n");

    // Setup sockets?

    // Build socket
    int client_fd = socket(PF_INET,
                           SOCK_STREAM,
                           IPPROTO_TCP);
    if (client_fd == -1)
        DIE("Couldn't create socket");

    // Connect socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(client_fd, (struct sockaddr *)(&addr), sizeof(addr)) < 0)
        DIE("Couldn't connect socket");

    while(1)
    {
        char buffer[1024]; // NOTE(chris): Why did this error with char *buffer?
        int num_bytes_read;
        while((num_bytes_read = read(client_fd, buffer, 1024)) == 0) {}
        if(num_bytes_read < 0)
            DIE("Couldn't read socket");

        printf("Num Bytes Read: %d\n", num_bytes_read);
        printf("Received: %s\n", buffer);
        buffer[0] = 0;

        char msg[] = "Glad to be here!\n";
        size_t msg_len = strlen(msg);
        if(write(client_fd, (void *)&msg, msg_len) != msg_len)
            DIE("Couldn't write");

    }

    if(close(client_fd) < 0)
        DIE("Couldn't close file descriptor");

    return 0;
}
