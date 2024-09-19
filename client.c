/********************************************************************************
 * Name        : client.c
 * Author      : Divya Prahlad
 * Pledge      : I pledge my honor that I have abided by the Stevens honor system
 *******************************************************************************/
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void parse_connect(int argc, char** argv, int* server_fd){
    int opt;
    char *qfile = "questions.txt";
    char *ip = "127.0.0.1";
    int pnum = 25555;
    while((opt = getopt(argc, argv, "f:i:p:h" ))  != -1){
        switch(opt){
            case 'f':
                qfile = optarg;
                break;
            case 'i':
                ip = optarg;
                break;
            case 'p':
                pnum = atoi(optarg);
                break;
            case 'h':
                printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", argv[0]);
                printf("  -f question_file    Default to \"questions.txt\";\n");
                printf("  -i IP_address       Default to \"127.0.0.1\";\n");
                printf("  -p port_number      Default to 25555;\n");
                printf("  -h                  Display this help info.\n");
                exit(EXIT_SUCCESS);
            case '?':
                if (optopt == 'f' || optopt == 'i' || optopt == 'p')
                {
                    fprintf(stderr, "Error: Option '-%c' requires an argument.\n", optopt);
                    exit(EXIT_FAILURE);
                }
                else{
                    fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                    exit(EXIT_FAILURE); 
                }
        }
    }

    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);

    /* STEP 1:
    Create a socket to talk to the server;
    */
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(*server_fd == -1){
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(25555);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* STEP 2:
    Try to connect to the server.
    */
    if(connect(*server_fd, (struct sockaddr *) &server_addr, addr_size) == -1 ){
        perror("Error connecting client to server");
        close(*server_fd);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]){

    int    server_fd;
    parse_connect(argc, argv, &server_fd);
    fd_set read_fds;
    

    char buffer[1024];

    while(1) {

        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(server_fd, &read_fds);

        int max_fd = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        
        /* Receive response from the server */
        if (FD_ISSET(server_fd, &read_fds)){
            int recvbytes = recv(server_fd, buffer, 1024, 0);
            if (recvbytes == 0){
                break;
            } 
            else {
                buffer[recvbytes] = 0;
                printf("%s", buffer); fflush(stdout);
            }
        }

        /* Read a line from terminal
           and send that line to the server
         */
        if (FD_ISSET(STDIN_FILENO, &read_fds)){
            fflush(stdout);
            scanf("%s", buffer);
            send(server_fd, buffer, strlen(buffer), 0);
        }

    }

    close(server_fd);

  return 0;
}
