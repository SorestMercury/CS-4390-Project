
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "definitions.h"


void main(int argc,char *argv[]){
	
   	char *server_address = "127.0.0.1";
	int server_port=5000;  // you should instead read from configuration file

	FILE *file = fopen("config.txt", "r");
	char buffer[256];
	while (fgets(buffer, sizeof(buffer), file) != NULL){
		printf("Read line: %s", buffer);
		/*char *linearg = strtok(buffer, " ");
		if (strcmp(linearg, "server_address") != 0) {
			*server_address = linearg[1];
		} else if (strcmp(linearg, "server_port") != 0) {
			server_port = linearg[1];
		}*/
	}
	fclose(file);
	printf("File read complete.");
	printf("hi");
   
    char cmd[128];

	while (strcmp(cmd, "EXIT") != 0){
		struct sockaddr_in server_addr;
		int sockid;

		char msg[1024];
			
		if ((sockid = socket(AF_INET,SOCK_STREAM,0))==-1){//create socket
			printf("socket cannot be created\n"); exit(0);
		}
												
	
		server_addr.sin_family = AF_INET;//host byte order
		server_addr.sin_port = htons(server_port);// convert to network byte order
		if (connect(sockid ,(struct sockaddr *) &server_addr,sizeof(struct sockaddr))==-1){//connect and error check
			printf("Cannot connect to server\n"); exit(0);
		}

		/* If connected successfully*/
	
		printf("Enter command: ");
		fgets(cmd, sizeof(cmd), stdin);
		cmd[strcspn(cmd, "\n")] = 0; // Remove the newline character
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
			const char* create_req="createtracker movie1.avi 109283519 Ghost_and_the_Darkness,_DVD_Rip c68c2ee8bfca4e898b396e7a935a1d92 202.141.60.10 2097\n";
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
		else if(strcmp(cmd, "GET")){
			//IMPLEMENT GET FUNCTIONALITY HERE
		}

		else if(strcmp(cmd, "UPDATETRACKER") == 0){
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
  
