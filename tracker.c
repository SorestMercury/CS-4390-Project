#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/dir.h>
#include <time.h>

const int MAXLINE=1024;
int sockid, sock_child;
int server_port=5000;
socklen_t clilen;

int peer_handler(int sock_child);

int main(){
	pid_t pid;
   struct sockaddr_in server_addr, client_addr;  
   if ((sockid = socket(AF_INET,SOCK_STREAM,0)) < 0){//create socket connection oriented
	   printf("socket cannot be created \n"); exit(0); 
   }
    
   //socket created at this stage
   //now associate the socket with local port to allow listening incoming connections
   server_addr.sin_family = AF_INET;// assign address family
   server_addr.sin_port = htons(server_port);//change server port to NETWORK BYTE ORDER
   server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    
   if (bind(sockid ,(struct sockaddr *) &server_addr,sizeof(server_addr)) ==-1){//bind and check error
	   printf("bind failure\n"); exit(0); 
   }
    
   printf("Tracker SERVER READY TO LISTEN INCOMING REQUEST.... \n");
   if (listen(sockid, 0) < 0){ //(parent) process listens at sockid and check error
	   printf(" Tracker  SERVER CANNOT LISTEN\n"); exit(0);
   }    
   clilen = sizeof(client_addr);                                    
   
   while(1) { //accept  connection from every requester client
	   if ((sock_child = accept(sockid ,(struct sockaddr *) &client_addr,&clilen))==-1){ /* accept connection and create a socket descriptor for actual work */
		   printf("Tracker Cannot accept...\n"); exit(0); 
	   }

	   if ((pid=fork())==0){//New child process will serve the requester client. separate child will serve separate client
		   close(sockid);   //child does not need listener
		   peer_handler(sock_child);//child is serving the client.		   
		   close (sock_child);// printf("\n 1. closed");
		   exit(0);         // kill the process. child process all done with work
        }
	   close(sock_child);  // parent all done with client, only child will communicate with that client from now
	   
     }  //accept loop ends            
} // main fun ends  

typedef struct{
	char fileName[256];
	long fileSize;
	char description[256];
	char md5[33];
	char ip[16];
	int port;
}createReq;

typedef struct{
	char fileName[256];
	long startByte;
	long endByte;
	char ip[16];
	int port;
}updateReq;

//prototypes
createReq tokenize_createmsg(const char *msg);
void handle_createtracker_req(int sock_child, createReq tokens);
updateReq tokenize_updatemsg(const char* msg);
void handle_updatetracker_req(int sock_child, updateReq tokens);

int peer_handler(int sock_child){ // function for file transfer. child process will call this function     
    //start handiling client request	
	int length;
	char read_msg[MAXLINE];
	char fname[64];
	length=read(sock_child,read_msg,MAXLINE);			
	read_msg[length]='\0';
	if((!strcmp(read_msg, "REQ LIST"))||(!strcmp(read_msg, "REQ LIST\n"))||(!strcmp(read_msg, "req list"))||(!strcmp(read_msg, "<REQ LIST>"))||(!strcmp(read_msg, "<REQ LIST>\n"))){//list command received
		//handle_list_req(sock_child);// handle list request
		const char* dummyResponse = "server acknowledges list request";
		write(sock_child,dummyResponse,strlen(dummyResponse));
		printf("list request handled.\n");
	}
	else if((strstr(read_msg,"get")!=NULL)||(strstr(read_msg,"GET")!=NULL)){// get command received
		//xtrct_fname(read_msg, " ");// extract filename from the command		
		//handle_get_req(sock_child, fname);		
	}
	else if((strstr(read_msg,"createtracker")!=NULL)||(strstr(read_msg,"Createtracker")!=NULL)||(strstr(read_msg,"CREATETRACKER")!=NULL)){// get command received
		createReq tokens = tokenize_createmsg(read_msg);
		handle_createtracker_req(sock_child, tokens);
		exit(0);
		
	}
	else if((strstr(read_msg,"updatetracker")!=NULL)||(strstr(read_msg,"Updatetracker")!=NULL)||(strstr(read_msg,"UPDATETRACKER")!=NULL)){// get command received
		updateReq tokens = tokenize_updatemsg(read_msg);
		handle_updatetracker_req(sock_child, tokens);
		exit(0);		
	}
	
}//end client handler function

int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

createReq tokenize_createmsg(const char* msg){
	createReq request;

	char buf[1024];
    strncpy(buf, msg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

	char *save;
	
	char *cmd = strtok_r(buf, " \t\r\n<>",&save);
    char *fn  = strtok_r(NULL, " \t\r\n<>",&save);
    char *sz  = strtok_r(NULL, " \t\r\n<>",&save);
    char *dsc = strtok_r(NULL, " \t\r\n<>",&save);
    char *md5 = strtok_r(NULL, " \t\r\n<>",&save);
    char *ip  = strtok_r(NULL, " \t\r\n<>",&save);
    char *prt = strtok_r(NULL, " \t\r\n<>",&save);

	if(!cmd || !fn || !sz || !dsc || !md5 || !ip || !prt){
		perror("missing token from create_tracker request");
	}

	strncpy(request.fileName, fn, 255);
	request.fileName[255] = '\0';

	request.fileSize = strtol(sz, NULL, 10);

	strncpy(request.description,dsc,255);
	request.description[255] = '\0';
	
	strncpy(request.md5, md5, 32);
	request.md5[32] = '\0';

	strncpy(request.ip,ip,15);
	request.ip[15] = '\0';
	
	request.port = strtol(prt, NULL, 10);

	return request;
}

void handle_createtracker_req(int sock_child, createReq tokens){

	char fileName[sizeof(tokens.fileName) + 15];
	snprintf(fileName, sizeof(fileName), "torrents/%s.track", tokens.fileName);

	if(!file_exists(fileName)){
		FILE *file = fopen(fileName,"w");

		if (file == NULL) {
			perror("Error opening or creating the file!");
		}

		time_t now = time(NULL);

		fprintf(file, "Filename:    %s\n", tokens.fileName);
		fprintf(file, "Filesize:    %ld\n", tokens.fileSize);
		fprintf(file, "Description: %s\n", tokens.description);
		fprintf(file, "Md5:         %s\n", tokens.md5);
		fprintf(file, "%s:", tokens.ip);
		fprintf(file, "%d:0:", tokens.port);
		fprintf(file, "%ld:", tokens.fileSize);
		fprintf(file, "%ld\n", (long)now);

		fclose(file);

		const char* msgPeer = "<createtracker succ>";
		printf("%s",msgPeer);
		write(sock_child,msgPeer,strlen(msgPeer));
		close(sock_child);
	}

	else{
		const char* msgPeer = "<createtracker ferr>";
		printf("%s",msgPeer);
		write(sock_child,msgPeer,strlen(msgPeer));
		close(sock_child);
	}
}

updateReq tokenize_updatemsg(const char* msg){
	updateReq request;

	char buf[1024];
    strncpy(buf, msg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

	char *save;
	
	char *cmd = strtok_r(buf, " \t\r\n<>",&save);
    char *fn  = strtok_r(NULL, " \t\r\n<>",&save);
    char *sb  = strtok_r(NULL, " \t\r\n<>",&save);
    char *eb = strtok_r(NULL, " \t\r\n<>",&save);
    char *ip  = strtok_r(NULL, " \t\r\n<>",&save);
    char *prt = strtok_r(NULL, " \t\r\n<>",&save);

	if(!cmd || !fn || !sb || !eb || !ip || !prt){
		perror("missing token from update_tracker request");
	}

	strncpy(request.fileName, fn, 255);
	request.fileName[255] = '\0';

	request.startByte = strtol(sb, NULL, 10);
	request.endByte = strtol(eb, NULL, 10);

	strncpy(request.ip,ip,15);
	request.ip[15] = '\0';
	
	request.port = strtol(prt, NULL, 10);

	return request;
}

void handle_updatetracker_req(int sock_child, updateReq tokens){
	
	char fileName[sizeof(tokens.fileName) + 15];
	snprintf(fileName, sizeof(fileName), "torrents/%s.track", tokens.fileName);

	if(file_exists(fileName)){

		if(file_exists(fileName)){

		FILE *file = fopen(fileName,"w");

		if (file == NULL) {
			perror("Error opening or creating the file!");
		}

		time_t now = time(NULL);

		typedef struct { long start, end; } range;
		char key[64];
		range ranges[128];
		char line[256];

		snprintf(key, sizeof(key), "%s:%d", tokens.ip, tokens.port);

		while(fgets(line,sizeof(line),file)){
			if(strncmp(line, key, strlen(key) == 0)){

				long start, end;
				sscanf(line+strlen(key), "%ld:%ld",start,end);

			}
		}

	}

	}
}