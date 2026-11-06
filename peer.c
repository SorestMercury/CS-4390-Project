
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
    
   
	struct sockaddr_in server_addr;
	int sockid;

	char msg[256];
        
	if ((sockid = socket(AF_INET,SOCK_STREAM,0))==-1){//create socket
		printf("socket cannot be created\n"); exit(0);
	}
                                              
   
    server_addr.sin_family = AF_INET;//host byte order
    server_addr.sin_port = htons(server_port);// convert to network byte order
    if (connect(sockid ,(struct sockaddr *) &server_addr,sizeof(struct sockaddr))==-1){//connect and error check
		printf("Cannot connect to server\n"); exit(0);
	}

   /* If connected successfully*/
	int list = 0;
	int createTracker = 1;
	if(list){// if this is LIST command
		// I changed this to 1 for now so that its always true for testing purposes. I will later handle running this block with proper timing
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
		
		close(sockid);
		printf("Connection closed\n");
	    exit(0);
	}//end close

	else if(createTracker){
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
		
		close(sockid);
		printf("Connection closed\n");
	    exit(0);
	}

}	     
  
