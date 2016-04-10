#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "safeio.h"

#define MAX_CLIENTS 64
#define BUFFER_SIZE 4096
#define BANNER "\n"\
               "   _____ _           _   _____       _ _\n"\
               "  / ____| |         | | |  __ \\     | | |\n"\
               " | |    | |__   __ _| |_| |__) |___ | | |\n"\
               " | |    | '_ \\ / _` | __|  _  // _ \\| | |\n"\
               " | |____| | | | (_| | |_| | \\ \\ (_) | | |\n"\
               "  \\_____|_| |_|\\__,_|\\__|_|  \\_\\___/|_|_|\n"\
               "\n"


// TODO:
//   - Implement better data scructure for client_fds
//   - Ascii art banner for welcome message?
//   - User names
//   - Broadcast to other clients when a client exits
//   - General cleanup

int server_fd;
int client_fds[MAX_CLIENTS] = {0}; // use 0 to denote abscence
int num_connected = 0;

void exit_gracefully(int signum)
{
    fprintf(stderr, "Closing server socket...\n");
    if(close(server_fd) < 0)
        DIE("Couldn't close server socket");
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(client_fds[i] == 0)
            continue;
        fprintf(stderr, "Closing sockets %d...\n", client_fds[i]);
        if(close(client_fds[i]) < 0)
            DIE("Couldn't close client socket");
    }
    exit(0);
}


void disconnect_client(int sock)
{
    fprintf(stderr, "Disconnecting %d...\n", sock);
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(client_fds[i] == 0)
            continue;
        if(client_fds[i] == sock)
            client_fds[i] = 0;
    }
    close(sock);
}

// Handles a writing to a socket that is disconnected.
void safe_write(int sock, char *msg)
{
    size_t msg_len = strlen(msg);
    if(write(sock, (void *)msg, msg_len) < 0)
        disconnect_client(sock);
}

// Handles reading form a disconnected socket.
int safe_read(int sock, char *msg)
{
    int len;
    if((len = read(sock, (void *)msg, BUFFER_SIZE)) < 0)
        DIE("Couldn't read_from client socket");
    if(len == 0) // This signals client disconnect. TODO(chris): Find a better way?
    {
        disconnect_client(sock);
        return -1;
    }
    else
    {
        msg[len] = 0;
        return 0;
    }
}

void broadcast(int sock, char *msg)
{
    fprintf(stderr, "Broadcasting: %s\n", msg);

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(client_fds[i] == 0 || client_fds[i] == sock)
            continue;  // Skip the sender
        safe_write(client_fds[i], msg);
    }
}


int main(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("Too few arguments.\n");
        printf("Usage: %s <port>\n\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);

    // Register signal handlers
    signal(SIGINT, exit_gracefully);


    printf("-------- ChatRoll Server --------\n");



    //////////////// Setup Server Socket //////////////////

    // Build socket
    fprintf(stderr, "Setting up server socket.\n");
    if ((server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        DIE("Couldn't create socket");

    // Bind socket
    fprintf(stderr, "Binding server socket.\n");

    int value = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)
        DIE("setsockopt failed");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0"); // Listen on all
    if(bind(server_fd, (struct sockaddr *)(&addr), sizeof(addr)) < 0)
        DIE("Couldn't bind socket");

    // Set socket to listen
    fprintf(stderr, "Setting server socket to listen.\n");
    if(listen(server_fd, 1) < 0)
        DIE("Couldn't listen");


    fprintf(stderr, "Server ready. Waiting for clients to connect.\n");



    //////////////// Main Loop //////////////////////////////

    fd_set rd_set;
    char msg[BUFFER_SIZE];
    while(1)
    {
        // Use select to wait for message form server or client sockets
        FD_ZERO (&rd_set);
        FD_SET (server_fd, &rd_set);
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            if(client_fds[i] == 0)
                continue;
            FD_SET (client_fds[i], &rd_set);
        }
        select(1024, &rd_set, NULL, NULL, NULL);

        // Message from server socket means new connection.
        if(FD_ISSET(server_fd, &rd_set))
        {
            if((client_fds[num_connected] = accept(server_fd, NULL, NULL)) < 0)
                DIE("Couldn't accept client socket");

            fprintf(stderr, "%d connected.\n", client_fds[num_connected]);

            safe_write(client_fds[num_connected], "Welcome to\n");
            safe_write(client_fds[num_connected], BANNER);
            safe_write(client_fds[num_connected], "Start your chatting!\n\n");

            broadcast(client_fds[num_connected], "A new client joined!\n");

            ++num_connected;
        }

        // Broadcast message from client to other clients
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            if(client_fds[i] == 0)
                continue;
            if(FD_ISSET(client_fds[i], &rd_set))
            {
                fprintf(stderr, "%d is ready for reading.\n", client_fds[i]);
                if(safe_read(client_fds[i], msg) == 0)
                    broadcast(client_fds[i], msg);
            }
        }

    }

    return 0;
}
