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
int server_port=5001;
socklen_t clilen;

int peer_handler(int sock_child);

int main(){
	pid_t pid;
   struct sockaddr_in server_addr, client_addr;  
   if ((sockid = socket(AF_INET,SOCK_STREAM,0)) < 0){//create socket connection oriented
	   printf("socket cannot be created \n"); exit(0); 
   }

   // Read server port from configuration file
	FILE *file = fopen("sconfig.cfg", "r");
	char buffer[256];
	while (fgets(buffer, sizeof(buffer), file) != NULL){
		char *linearg = strtok(buffer, " ");
		if(strcmp(linearg, "PORT") == 0) {
			server_port = atoi(strdup(strtok(NULL, " \n")));
		}
	}
	fclose(file);
    
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
void xtract_fname(const char *msg, char* fname);
void handle_get_req(int sock_child, const char* fname);

int peer_handler(int sock_child){ // function for file transfer. child process will call this function     
    //start handiling client request	
	int length;
	char read_msg[MAXLINE];
	char fname[64];
	length=read(sock_child,read_msg,MAXLINE);			
	read_msg[length]='\0';
	if((!strcmp(read_msg, "REQ LIST"))||(!strcmp(read_msg, "REQ LIST\n"))||(!strcmp(read_msg, "req list"))||(!strcmp(read_msg, "<REQ LIST>"))||(!strcmp(read_msg, "<REQ LIST>\n"))){//list command received
		handle_list_req(sock_child);
		printf("list request handled.\n");
	}
	else if((strstr(read_msg,"get")!=NULL)||(strstr(read_msg,"GET")!=NULL)){// get command received
		xtract_fname(read_msg, fname); //extract filename from the command	
		printf("%s\n",fname);	
		handle_get_req(sock_child, fname);		
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

	//copy message into buffer
	char buf[1024];
    strncpy(buf, msg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

	char *save; //save pointer for threadsafe strtok
	
	//split message into tokens
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

	//store attributes inside of createReq
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

	//append .track to file to check if it exists in true directory
	char fileName[sizeof(tokens.fileName) + 15];
	snprintf(fileName, sizeof(fileName), "torrents/%s.track", tokens.fileName);

	if(!file_exists(fileName)){
		//open file
		FILE *file = fopen(fileName,"w");

		if (file == NULL) {
			perror("Error opening or creating the file!");
		}

		time_t now = time(NULL);

		//write information from tokens into file
		fprintf(file, "Filename:    %s\n", tokens.fileName);
		fprintf(file, "Filesize:    %ld\n", tokens.fileSize);
		fprintf(file, "Description: %s\n", tokens.description);
		fprintf(file, "Md5:         %s\n", tokens.md5);
		fprintf(file, "%s:", tokens.ip);
		fprintf(file, "%d:0:", tokens.port);
		fprintf(file, "%ld:", tokens.fileSize);
		fprintf(file, "%ld\n", (long)now);

		fclose(file);

		//inform client of success
		const char* msgPeer = "<createtracker succ>";
		printf("%s\n",msgPeer);
		write(sock_child,msgPeer,strlen(msgPeer));
		close(sock_child);
	}

	//inform client of failure
	else{
		const char* msgPeer = "<createtracker ferr>";
		printf("%s\n",msgPeer);
		write(sock_child,msgPeer,strlen(msgPeer));
		close(sock_child);
	}
}

updateReq tokenize_updatemsg(const char* msg){
	updateReq request;

	//copy message into buffer
	char buf[1024];
    strncpy(buf, msg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

	char *save; //save pointer for threadsafe strtok
	
	//split message into tokens
	char *cmd = strtok_r(buf, " \t\r\n<>",&save);
    char *fn  = strtok_r(NULL, " \t\r\n<>",&save);
    char *sb  = strtok_r(NULL, " \t\r\n<>",&save);
    char *eb = strtok_r(NULL, " \t\r\n<>",&save);
    char *ip  = strtok_r(NULL, " \t\r\n<>",&save);
    char *prt = strtok_r(NULL, " \t\r\n<>",&save);

	if(!cmd || !fn || !sb || !eb || !ip || !prt){
		perror("missing token from update_tracker request");
	}

	//store attributes inside of updateReq
	strncpy(request.fileName, fn, 255);
	request.fileName[255] = '\0';

	request.startByte = strtol(sb, NULL, 10);
	request.endByte = strtol(eb, NULL, 10);

	strncpy(request.ip,ip,15);
	request.ip[15] = '\0';
	
	request.port = strtol(prt, NULL, 10);

	return request;
}

//range type for comparisons later
typedef struct { long start, end; } range;

//to compare ranges by start value for sorting
static int cmp_start(const void *a, const void *b) {
			long x = ((const range *)a)->start;
			long y = ((const range *)b)->start;
			return (x > y) - (x < y); 
		}

void handle_updatetracker_req(int sock_child, updateReq tokens){

	//append .track to file to check if it exists in true directory
	char fileName[sizeof(tokens.fileName) + 15];
	snprintf(fileName, sizeof(fileName), "torrents/%s.track", tokens.fileName);

	if(file_exists(fileName)){
		//open file
		FILE *file = fopen(fileName,"r");

		if (file == NULL) {
			perror("Error opening or creating the file!");
		}

		//maximum of 128 possible entries of same ip:port combo in tracker file
		range ranges[128];
		int rangen = 0;

		char line[1024];
		char key[64];

		//create temp path/file to modify instead of modifying original tracker file
		char tmpPath[512];
		snprintf(tmpPath, sizeof(tmpPath), "%s.XXXXXX", fileName);
		int fd = mkstemp(tmpPath);          
		if (fd < 0) { perror("mkstemp"); }

		FILE *temp = fdopen(fd, "w");        
		if (!temp) { perror("fdopen"); close(fd); remove(tmpPath); }

		//create unique ip:port combo to search for in line
		snprintf(key, sizeof(key), "%s:%d:", tokens.ip, tokens.port);
		size_t keylen = strlen(key);

		while(fgets(line,sizeof(line),file)){
			if(strncmp(line, key, strlen(key)) == 0){ //if line contains key

				long start, end;
				if (sscanf(line + keylen, "%ld:%ld", &start, &end) == 2 && rangen < 128) {
					//load start and end of range into array of ranges, increment to next spot in array
					ranges[rangen].start = start;
					ranges[rangen].end = end;
					rangen++;
				}
			}

			else //no key contained in line, write it back to temp file as is
				fputs(line, temp);                 
		}

		if(rangen<128) { //add the range from the updatetracker command to the array of ranges
			ranges[rangen].start = tokens.startByte; 
			ranges[rangen].end = tokens.endByte;
			rangen++;
		}

		qsort(ranges, rangen, sizeof(range), cmp_start); //sort the array of ranges by start time

		long start = ranges[0].start;
		long end = ranges[0].end;
		time_t now = time(NULL);

		for(int i = 1;i<rangen;i++){ //iterate through array, build merged contiguous ranges for ranges that overlap

			if(ranges[i].start <= end){
				if(ranges[i].end > end)
					end = ranges[i].end;
			}

			else {
				fprintf(temp, "%s%ld:%ld:%ld\n", key, start, end, now); //if this range doesnt overlap with the previous, write current merged range to file, start new merge
				start = ranges[i].start;
				end   = ranges[i].end;
        	}
		}

		fprintf(temp, "%s%ld:%ld:%ld\n", key, start, end, now); //write final merged range to file
		fclose(temp);
		fclose(file);
		
		char msgPeer[1024];

		//replace current trackerfile with temp file, notify client of success/fail
		if (rename(tmpPath, fileName) != 0) {
			perror("rename");
			remove(tmpPath);
			snprintf(msgPeer, 1024, "<updatetracker %s fail>", tokens.fileName);
			printf("%s",msgPeer);
		} 
		
		else {
			snprintf(msgPeer, 1024, "<updatetracker %s succ>", tokens.fileName);
		}

		printf("%s",msgPeer);
		write(sock_child,msgPeer,strlen(msgPeer));
		close(sock_child);
	}
}

void xtract_fname(const char *msg, char* fname) {
    int i = 0;
    while (msg[i+4] != '\0' && msg[i+4] != '\n') { //GET filename.track
        fname[i] = msg[i+4];
        i++;
    }
    fname[i] = '\0';
    return;
}

void handle_get_req(int sock_child, const char* fname) {
        printf("REP GET BEGIN\n");
        send(sock_child, "REP GET BEGIN", 14, 0);
        char buff[256];
        int n = recv(sock_child, buff, sizeof(buff)-1, 0);
		buff[n] = '\0';
        //printf("CONFIRM\n");
        char flocation[256];
        sprintf(flocation, "torrents/%s", fname);
	FILE* fptr = fopen(flocation, "r");
	if (fptr == NULL) {
	  printf("FILE FAILED TO OPEN\n");
	  exit(0);	
	}
	char line[MAXLINE];
	while (fgets(line, MAXLINE, fptr)) { //read file one line at a time
		//write to socket
		printf("%s", line);
		send(sock_child,line,strlen(line), 0);
		char buff[100];
		recv(sock_child, buff, 100, 0);
		//printf("CONFIRM\n");
	}
	fclose(fptr);
	printf("REP GET END md5\n"); //IMPLEMENT AND APPEND FILE MD5
	send(sock_child, "REP GET END md5", 16, 0);
	close(sock_child);
	return;
}

void handle_list_req(int sock_child) {
    DIR *dir; //directory pointer
    struct dirent *entry; // entry pointer
    FILE *file;// file pointer

    char path[512]; //store path to file
    char line [512]; //store line read from file
    char response[MAXLINE]; //store response for peer

    int used = 0; //characters written to response buffer
    int found = 0; //flag file found
    response[0] = '\0'; //empty response string

    dir = opendir("torrents"); //open torrents directory
    //validate if directory is accessible, send error to peer
    if (dir == NULL) {
        snprintf(response, sizeof(response), "No tracker directory found.\n");
        write(sock_child, response, strlen(response));
        return;
    }

    used = snprintf(response, sizeof(response), "Available files:\n"); //add response heading

    while ((entry = readdir(dir)) != NULL) {// read each entry of directory
        int name_len = strlen(entry->d_name); //current file length
        if (name_len < 6 || strcmp(entry->d_name + name_len - 6, ".track") != 0) { //validate length
            continue;  //skip files without metadata
        }
        //build metadata file path
        snprintf(path, sizeof(path), "torrents/%s", entry->d_name);
        //open metadata file to read
        file = fopen(path, "r");
        //skip files that cannot be opened
        if (file == NULL) {
            continue;
        }
        //successfull metadata file
        found = 1;

        //read 3 lines for filename, filesize, and description
        for (int i = 0; i < 3; i++) {
            if (fgets(line, sizeof(line), file) == NULL) {
                break;  //stop at end of lines
            }
            //check for space in response buffer
            if (used < (int)sizeof(response) - 1) {
                int remaining = (int)sizeof(response) - used; //append line to response
                int written = snprintf(response + used, remaining, "%s", line);
                //update characters used
                if (written < 0) {
                    break;
                }
                //ensure not exceeding buffer size
                if (written >= remaining) {
                    used = sizeof(response) - 1;
                    break;
                }
                used += written;
            }
        }

        //separate file entries with a newline
        if (used < (int)sizeof(response) - 1) {
            int remaining = (int)sizeof(response) - used;
            int written = snprintf(response + used, remaining, "\n");
            if (written > 0 && written < remaining) {
                used += written;
            }
        }
        fclose(file);
    } //close file and directory when done
    closedir(dir);
    // alert peer if no file found
    if (!found) {
        snprintf(response, sizeof(response), "No files are currently registered.\n");
    }
    // send information to peer
    write(sock_child, response, strlen(response));
}
