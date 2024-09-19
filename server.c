/********************************************************************************
 * Name        : server.c
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

#define MAX_CONN 3

struct Player {
    int fd;
    int score;
    char name[128];
};
struct Player players[MAX_CONN];

struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};
struct Entry entries[50];

int read_questions(struct Entry* arr, char* filename){
    FILE* file = fopen(filename, "r");
    if(!file){
        perror("Error opening file");
        return 0;
    }

    int num_entries = 0;
    char line[1024];

    while(fgets(line, sizeof(line), file)){
        line[strcspn(line, "\n")] =  '\0';
        if(strlen(line) ==0) continue;
        strcpy(arr[num_entries].prompt, line);

        fgets(line, sizeof(line), file);
        sscanf(line, "%s %s %s", arr[num_entries].options[0], arr[num_entries].options[1], arr[num_entries].options[2]);


        fgets(line, sizeof(line), file);
        line[strcspn(line, "\n")] = '\0';
        for(int i = 0; i<3;  i++){
            if(strcmp(line, arr[num_entries].options[i]) == 0){
                arr[num_entries].answer_idx = i;
                break;
            }
        }

        for(int i = 0; i<3; i++){
            for(int j = 0; j< strlen(arr[num_entries].options[i]); j++){
                if(arr[num_entries].options[i][j] == '_')
                    arr[num_entries].options[i][j] = ' ';
            }
        }
        num_entries ++;


    }

    fclose(file);
    return num_entries;
}
void determine_winners(struct Player* players){
    //determine winner
    int max_score = -50;
    char winners[MAX_CONN][128];
    int num_winners=0;
    for(int i = 0; i< MAX_CONN; i++){
        if(players[i].fd != -1 && players[i].score >= max_score){
            if(players[i].score > max_score){
                max_score = players[i].score;
                num_winners = 0;
            }
            strcpy(winners[num_winners], players[i].name);
            num_winners++;
        }
    }
    if(num_winners == 1)
        printf("Congrats, %s! You win!\n", winners[0]);
    else{
        printf("Its a tie!\nCongrats, Winners:\n");
        for(int i = 0; i<num_winners; i++){
            printf("%s\n", winners[i]);
        }
    }
}

void print_entry(struct Entry* arr, int num_entry) {
    printf("Question %d: %s\n", num_entry+1, arr[num_entry].prompt);
    printf("1: %s\n2: %s\n3: %s\n", arr[num_entry].options[0], arr[num_entry].options[1], arr[num_entry].options[2]);
}

int main(int argc, char* argv[]){
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
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                exit(EXIT_FAILURE); 
        }
    }
    int num_q = read_questions(entries, qfile);
    int    server_fd;
    int    client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);

    /* STEP 1
        Create and set up a socket
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(pnum);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    /* STEP 2
        Bind the file descriptor with address structure
        so that clients can find the address
    */
    int i =  
    bind(server_fd,
            (struct sockaddr *) &server_addr,
            sizeof(server_addr));
    if (i < 0) {perror("bind"); close(server_fd);  exit(1);}

    /* STEP 3
        Listen to at most 3 incoming connections
    */
    if (listen(server_fd, MAX_CONN) == 0)
        printf("Welcome to 392 Trivia!\n");
    else{
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    } 

    /* STEP 4
        Accept connections from clients
        to enable communication
    */

    fd_set myset;
    FD_SET(server_fd, &myset);
    int maxfd = server_fd;
    int n_conn = 0;
    
    int cfds[MAX_CONN];
    for(int i = 0; i<MAX_CONN; i++) cfds[i] = -1;
    for(int i = 0; i<MAX_CONN; i++){
        players[i].fd = -1;
        players[i].score = 0;
    }

    char* receipt1   = "Please wait for the game to begin!\n";
    char* receipt2   = "You are the final player!\n";
    int   recvbytes = 0;
    char  buffer[2048];
    int num_names = 0;
    int not_printed = 1;
    int curr_q = 0;
    while(1){

        //re-initialize fd_set
        FD_SET(server_fd, &myset);
        maxfd = server_fd;
        for(int i = 0; i<MAX_CONN; i++){
            if(players[i].fd != -1) {
                FD_SET(players[i].fd, &myset);
                if(players[i].fd > maxfd) maxfd = players[i].fd;
            }
        }

        //monitor file descriptor set
        if(select(maxfd+1, &myset, NULL, NULL, NULL) == -1){
            perror("select");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        
        //check if a new connection is coming in
        if(FD_ISSET(server_fd, &myset)){
            client_fd = accept(server_fd,
                                (struct sockaddr*)&in_addr,
                                &addr_size);
            if (client_fd == -1) {
                perror("Error accepting client connection");
                exit(EXIT_FAILURE);
            }
            if(n_conn < MAX_CONN) {
                n_conn++;
                printf("New Connection detected!\n");
                for(int i = 0; i< MAX_CONN; i++){
                    if(players[i].fd == -1){
                        players[i].fd = client_fd;   
                        break;
                    }
                }
                write(client_fd, "Please type your name: ", 23);
                
            }
            else{ 
                printf("Max connections reached!\n");
                close(client_fd); 
            }
        }

        for(int i = 0; i<MAX_CONN; i++){ 
            if(players[i].fd != -1 && FD_ISSET(players[i].fd, &myset)){
                recvbytes = read(players[i].fd, buffer, 1024);
                if(recvbytes == 0){
                    printf("Lost Connection!\n");
                    close(players[i].fd);
                    n_conn --;
                    for(int j = 0; j<MAX_CONN; j++)
                        players[j].fd = -1;
                    return 1;    
                }else if(recvbytes == -1){
                    perror("Error reading socket");
                    close(server_fd);
                    exit(EXIT_FAILURE);
                }
                else{
                    buffer[recvbytes] = 0;
                    char *endptr;
                    long int temp = strtol(buffer, &endptr, 10);
                    if(*endptr =='\0'){
                        int guess = (int)temp;
                        char *answ = entries[curr_q-1].options[entries[curr_q-1].answer_idx];
                        char message[256];
                        sprintf(message, "Correct Answer: %s\n", answ);
                        if(guess-1 == entries[curr_q-1].answer_idx){
                            players[i].score++;
                            printf("Correct!\n");

                        }
                        else{
                            players[i].score--;
                            printf("Incorrect!\n");
                        }
                        for(int i = 0; i<MAX_CONN; i++){
                            write(players[i].fd, message, strlen(message));
                        }
                        printf("\nCurrent Scores:\n");
                        for(int i = 0; i<MAX_CONN-1;  i++){
                            printf("%s: %d ~ ", players[i].name, players[i].score);
                        }
                        printf("%s: %d\n\n", players[MAX_CONN-1].name, players[MAX_CONN-1].score);

                    }else{ 
                        strncpy(players[i].name, buffer, sizeof(players[i].name) - 1);
                        printf("Hi %s!\n", players[i].name);
                        num_names ++;

                    }
                }
            }
        
        }

        if(num_names == MAX_CONN){
            if(not_printed){
                printf("The game starts now!\n");
                not_printed = 0;
            }
            if(curr_q == num_q)
                break;
            print_entry(entries, curr_q);
            char opts[2048];
            for (int i = 0; i < MAX_CONN; i++)
            {
                if (players[i].fd != -1)
                { 
                    sprintf(buffer, "Question %d: %s\n", curr_q + 1, entries[curr_q].prompt);
                    write(players[i].fd, buffer, strlen(buffer));

                    sprintf(opts, "Press 1: %s\nPress 2: %s\nPress 3: %s\n", entries[curr_q].options[0], entries[curr_q].options[1], entries[curr_q].options[2]);
                    write(players[i].fd, opts, strlen(opts));
                }
            }
            curr_q++;


            
        }

    }

    determine_winners(players);

    close(client_fd);
    close(server_fd);
    return 0;
}
