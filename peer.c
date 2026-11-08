
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include "definitions.h"

void xtract_fname(const char *msg, char* fname); //used with GET

void main(int argc,char *argv[]){
	
   	char *server_address = "127.0.0.1";
	int server_port=5001;  // you should instead read from configuration file

	// Read server address and port from configuration file
	FILE *file = fopen("config.txt", "r");
	char buffer[256];
	while (fgets(buffer, sizeof(buffer), file) != NULL){
		char *linearg = strtok(buffer, " ");
		if (strcmp(linearg, "server_ip") == 0) {
			server_address = strdup(strtok(NULL, " \r\n"));
		} else if (strcmp(linearg, "server_port") == 0) {
			server_port = atoi(strdup(strtok(NULL, " \r\n")));
		}
	}
	fclose(file);
   
    char cmd[256];
	char *args[10]; // Array to hold command arguments

	while (strcmp(cmd, "EXIT") != 0){
		struct sockaddr_in server_addr;
		int sockid;
		char msg[1024];
			
		if ((sockid = socket(AF_INET,SOCK_STREAM,0))==-1){//create socket
			printf("socket cannot be created\n"); exit(0);
		}
												
		server_addr.sin_family = AF_INET;//host byte order
		server_addr.sin_port = htons(server_port);// convert to network byte order

		if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) != 1) {
    		printf("Bad server address\n"); exit(0);
		}

		if (connect(sockid ,(struct sockaddr *) &server_addr,sizeof(struct sockaddr))==-1){//connect and error check
			printf("Cannot connect to server\n");
			printf("errno: %i\n", errno);
			exit(0);
		}

		/* If connected successfully*/
	
		printf("Enter command: ");
		fgets(cmd, sizeof(cmd), stdin);
		cmd[strcspn(cmd, "\n")] = 0; // Remove the newline character
		args[0] = strtok(cmd, " "); // Tokenize the command
		int i = 1;
		while ((args[i] = strtok(NULL, " ")) != NULL) {
			i++;
		}
		args[i] = NULL; // Null-terminate the array

		if(strcmp(cmd, "LIST") == 0){
			const char* list_req="REQ LIST";
			if((write(sockid,list_req,strlen(list_req))) < 0){//inform the server of the list request
				printf("Send_request  failure\n"); exit(0);
			}
		
			int n; //store length of read data
        	if((n = read(sockid,(char *)&msg,sizeof(msg)))< 0){// read what server has said
				printf("Read  failure\n"); exit(0); 
			}

			msg[n] = '\0'; //append null terminator to read message
    		printf("%s\n", msg);
		}
		
		else if(strcmp(cmd, "CREATETRACKER") == 0){
			char fileName[256], description[256], md5[33], ip[16];
			long fileSize;
			if (i < 2) {
				printf("Error: CREATETRACKER command requires 2 arguments.\n");
				continue;
			}
			FILE *file = fopen(args[1], "rb");
			if (file == NULL) {
				printf("Error: Could not open file %s\n", args[1]);
				continue;
			}
			fseek(file, 0, SEEK_END);
			fileSize = ftell(file);
			struct ifaddrs *ifaddr, *ifa;
			if (getifaddrs(&ifaddr) == -1) {
				perror("getifaddrs");
				exit(EXIT_FAILURE);
			}

			for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
				if (ifa->ifa_addr == NULL) continue;
				if (ifa->ifa_addr->sa_family == AF_INET) {
					struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
					inet_ntop(AF_INET, &(sa->sin_addr), ip, sizeof(ip));
					printf("IP Address: %s\n", ip);
					break;
				}
			}
			freeifaddrs(ifaddr);

			fclose(file);
			char create_req[600]; // 256 + 256 + 33 + 10 + 16 + extra for command and spaces
			//Format is: CREATETRACKER <filename> <filesize> <description> <md5> <ip> <port>
			sprintf(create_req, "createtracker %s %ld %s %s %s %d\n", args[1], fileSize, args[2], md5, ip, port);
			printf("Sending request: %s\n", create_req);
			if((write(sockid,create_req,strlen(create_req))) < 0){//inform the server of the create request
				printf("Send_request  failure\n"); exit(0);
			}

			int n; //store length of read data
        	if((n = read(sockid,(char *)&msg,sizeof(msg)))< 0){// read what server has said
				printf("Read  failure\n"); exit(0); 
			}

			msg[n] = '\0'; //append null terminator to read message
    		printf("%s\n", msg);
		}
		else if(strncmp(cmd, "GET", 3) == 0){ //get command
			//IMPLEMENT GET FUNCTIONALITY HERE
			send(sockid, cmd, strlen(cmd), 0);
			int n = recv(sockid, (char*)&msg, sizeof(msg), 0); //REP GET
			send(sockid, "CONFIRM", 7, 0);
			msg[n] = '\0';
			printf("%s\n", msg);
			char fn[64];
			xtract_fname(cmd, fn);
			char flocation[256];
                        sprintf(flocation, "cache/%s", fn);
	                FILE* fptr = fopen(flocation, "w");
	                if (fptr == NULL) {
                                printf("FILE FAILED TO OPEN\n");
                                exit(0);
                        }
                        int loop = 1;
                        while (loop > 0) {
                            n = recv(sockid, (char *)&msg, sizeof(msg), 0);
                            msg[n] = '\0';
                            printf("line %d: %s\n", loop, msg);
                            char* end = strstr(msg, "REP GET END");
                            if (end) {
                              //IMPLEMENT MD5 CHECK HERE
                              loop = 0;
                              end[0] = '\0';
                            }
                            else {
                              loop++;
                            }
                            fputs(msg, fptr);
                            send(sockid, "CONFIRM", 7, 0);
                        }
                        fclose(fptr);
		}

		else if(strcmp(cmd, "UPDATETRACKER") == 0){
			//Format is: UPDATETRACKER <filename> <filesize> <description> <md5> <ip>
			const char* create_req="updatetracker movie1.avi 109283521 109283530 202.141.60.10 2097\n";
			if((write(sockid,create_req,strlen(create_req))) < 0){//inform the server of the create request
				printf("Send_request  failure\n"); exit(0);
			}

			int n; //store length of read data
        	if((n = read(sockid,(char *)&msg,sizeof(msg)))< 0){// read what server has said
				printf("Read  failure\n"); exit(0); 
			}

			msg[n] = '\0'; //append null terminator to read message
    		printf("%s\n", msg);
		}

		close(sockid);
		//printf("Connection closed\n");
	}
	exit(0);
}	     

void xtract_fname(const char *msg, char* fname) {
    int i = 0;
    while (msg[i+4] != '\0') { //GET filename.track
        fname[i] = msg[i+4];
        i++;
    }
    fname[i] = '\0';
    return;
}
