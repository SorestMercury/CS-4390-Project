
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
#include <netdb.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include "definitions.h"

void xtract_fname(const char *msg, char* fname); //used with GET
char *calculate_md5(const char *filename); //used with CREATETRACKER
char *get_local_ip(); //used with CREATETRACKER and UPDATETRACKER
void alarm_handler(int sig); //used with UPDATETRACKER
void updateTracker(const char *filename);
void host_file(const char *filename, int port);
static int fetch_segment(const char *ip, int port, const char *name,
                         long start, long end, FILE *out);
static void handle_peer(int connfd, const char *filename);
static int write_all(int fd, const char *buf, size_t len);
static int read_line(int fd, char *buf, size_t max);
volatile sig_atomic_t sockid;
volatile sig_atomic_t server_port;

void main(int argc,char *argv[]){
   	char *server_address = "127.0.0.1";
	server_port=5001;
	int tracker_port=5001;
	int update_interval=900; // 15 minutes in seconds
	char shared_folder[256] = "./torrents";
	alarm(update_interval);
	//signal(SIGALRM, alarm_handler); //signal isnt working right now
	

	// Read server address and port from configuration file
	FILE *file = fopen("clientThreadConfig.cfg", "r");
	char buffer[256];
	while (fgets(buffer, sizeof(buffer), file) != NULL){
		char *linearg = strtok(buffer, " ");
		if (strcmp(linearg, "TRACKER_IP") == 0) {
			server_address = strdup(strtok(NULL, " \r\n"));
		} else if (strcmp(linearg, "TRACKER_PORT") == 0) {
			tracker_port = atoi(strdup(strtok(NULL, " \r\n")));
		} else if (strcmp(linearg, "UPDATE_INTERVAL_SECONDS") == 0) {
			update_interval = atoi(strdup(strtok(NULL, " \r\n")));
		}
	}
	fclose(file);
	file = fopen("serverThreadConfig.cfg", "r");
	while (fgets(buffer, sizeof(buffer), file) != NULL){
		char *linearg = strtok(buffer, " ");
		if (strcmp(linearg, "LISTEN_PORT") == 0) {
			server_port = atoi(strdup(strtok(NULL, " \r\n")));
		}
	}
	fclose(file);

    char cmd[256];
	char cmdTok[256]; //so that original command is not modified when tokenized, incase needed
	char *args[10]; // Array to hold command arguments

	while (strcmp(cmd, "EXIT") != 0){
		struct sockaddr_in server_addr;
		int sockid;
		char msg[1024];
			
		if ((sockid = socket(AF_INET,SOCK_STREAM,0))==-1){//create socket
			printf("socket cannot be created\n"); exit(0);
		}
												
		server_addr.sin_family = AF_INET;//host byte order
		server_addr.sin_port = htons(tracker_port);// convert to network byte order

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
		if (cmd[0] == '\n') {
			continue; // Skip empty commands
		}
		cmd[strcspn(cmd, "\n")] = 0; // Remove the newline character
		strcpy(cmdTok,cmd); //so that original command is not modified when tokenized, incase needed
		args[0] = strtok(cmdTok, " "); // Tokenize the command
		int i = 1;
		while ((args[i] = strtok(NULL, " \n\0")) != NULL) {
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
		
		else if(strcmp(args[0], "CREATETRACKER") == 0){
			char fileName[256], description[256], md5[33], ip[16];
			long fileSize;
			if (i < 2) {
				printf("Error: CREATETRACKER command requires 1 argument.\n");
				continue;
			}
			FILE *file = fopen(args[1], "r");
			if (file == NULL) {
				printf("Error: Could not open file %s\n", args[1]);
				continue;
			}
			fseek(file, 0, SEEK_END);
			fileSize = ftell(file);
			fclose(file);
			
			// Get the local IP address
			strcpy(ip, get_local_ip());

			// Get md5 hash of the file
			strncpy(md5, calculate_md5(args[1]), sizeof(md5));
			
			char create_req[600]; // 256 + 256 + 33 + 10 + 16 + extra for command and spaces
			//Format is: CREATETRACKER <filename> <filesize> <description> <md5> <ip> <port>
			sprintf(create_req, "createtracker %s %ld %s %s %s %d\n", args[1], fileSize, args[2], md5, ip, server_port);
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
		
		else if(strncmp(args[0], "GET", 3) == 0){ //get command
			//IMPLEMENT GET FUNCTIONALITY HERE
			printf("%s",cmd);
			send(sockid, cmd, strlen(cmd), 0);
			int n = recv(sockid, (char*)&msg, sizeof(msg), 0); //REP GET
			send(sockid, "CONFIRM", 7, 0);
			msg[n] = '\0';
			printf("%s\n", msg);
			char flocation[256];
                        sprintf(flocation, "cache/%s", args[1]);
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

		else if(strcmp(args[0], "UPDATETRACKER") == 0){
			char update_req[600], ip[16];
			long fileSize;
			if (i < 2) {
				printf("Error: UPDATETRACKER command requires 1 argument.\n");
				continue;
			}
			
			//opening file to get filesize
			FILE *file = fopen(args[1], "r");
			if (file == NULL) {
				printf("Error: Could not open file %s\n", args[1]);
				continue;
			}
			fseek(file, 0, SEEK_END);
			fileSize = ftell(file);
			fclose(file);

			//Format is: UPDATETRACKER <filename> <start_byte> <end_byte> <ip> <port>
			strcpy(ip, get_local_ip());
			sprintf(update_req, "updatetracker %s %d %ld %s %d\n", args[1], 0, fileSize, ip, server_port);
			printf("Sending request: %s\n", update_req);
			if((write(sockid,update_req,strlen(update_req))) < 0){//inform the server of the update request
				printf("Send_request  failure\n"); exit(0);
			}

			int n; //store length of read data
        	if((n = read(sockid,(char *)&msg,sizeof(msg)))< 0){// read what server has said
				printf("Read  failure\n"); exit(0); 
			}

			msg[n] = '\0'; //append null terminator to read message
    		printf("%s\n", msg);
		}

		else if (strcmp(args[0], "DOWNLOAD") == 0) {
			if (i < 6) {
				printf("Usage: DOWNLOAD <ip> <port> <filename> <start> <end>\n");
				continue;
			}

			char *ip = args[1];
			int port = atoi(args[2]);
			char *name = args[3];
			long start = atol(args[4]);
			long end = atol(args[5]);

			char outpath[300];
			snprintf(outpath, sizeof(outpath), "media/%s", name);

			FILE *out = fopen(outpath, "r+b");
			if (out == NULL) out = fopen(outpath, "w+b");
			if (out == NULL) {
				printf("Error: cannot open %s\n", outpath);
				continue;
			}

			if (fetch_segment(ip, port, name, start, end, out) == 0)
				printf("Got bytes %ld-%ld into %s\n", start, end, outpath);
			else
				printf("fetch_segment failed\n");

			fclose(out);
		}

		else if (strcmp(args[0], "HOST") == 0 && i >= 3) {
			host_file(args[1], atoi(args[2]));
		}

		close(sockid);
		//printf("Connection closed\n");
	}
	exit(0);
}	     

void host_file(const char *filename, int port) {
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return;
	}
	if (pid > 0) {
		printf("Hosting %s on port %d (pid %d)\n", filename, port, pid);
		return;
	}

	//child
	signal(SIGCHLD, SIG_IGN); //auto-reap per-connection handlers, no zombies

	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		perror("socket");
		exit(1);
	}

	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("bind");
		exit(1);
	}
	if (listen(listenfd, 10) == -1) {
		perror("listen");
		exit(1);
	}

	while (1) {
		struct sockaddr_in cli;
		socklen_t clilen = sizeof(cli);
		int connfd = accept(listenfd, (struct sockaddr *)&cli, &clilen);
		if (connfd == -1) {
			if (errno == EINTR) continue;
			perror("accept");
			continue;
		}

		if (fork() == 0) {
			close(listenfd);
			handle_peer(connfd, filename);
			close(connfd);
			exit(0);
		}
		close(connfd);
	}
}

static void handle_peer(int connfd, const char *filename) {
	char req[512], reqName[256];
	long start, end;

	if (read_line(connfd, req, sizeof(req)) <= 0) return;

	if (sscanf(req, "SEND %255s %ld %ld", reqName, &start, &end) != 3) {
		write_all(connfd, "ERROR bad request\n", 18);
		return;
	}
	if (strcmp(reqName, filename) != 0) {
		write_all(connfd, "ERROR unknown file\n", 19);
		return;
	}

	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		write_all(connfd, "ERROR cannot open file\n", 23);
		return;
	}

	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	if (start < 0 || end > fileSize || start >= end) {
		write_all(connfd, "ERROR bad range\n", 16);
		fclose(fp);
		return;
	}

	fseek(fp, start, SEEK_SET);
	char buf[1024];
	long remaining = end - start;
	while (remaining > 0) {
		size_t want = remaining < (long)sizeof(buf) ? remaining : sizeof(buf);
		size_t got = fread(buf, 1, want, fp);
		if (got == 0) break;
		if (write_all(connfd, buf, got) < 0) break;
		remaining -= got;
	}
	fclose(fp);
}

static int fetch_segment(const char *ip, int port, const char *name,
                         long start, long end, FILE *out) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) return -1;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1 ||
	    connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		close(fd);
		return -1;
	}

	char req[512];
	snprintf(req, sizeof(req), "SEND %s %ld %ld\n", name, start, end);
	if (write_all(fd, req, strlen(req)) < 0) { close(fd); return -1; }

	fseek(out, start, SEEK_SET);
	long remaining = end - start;
	char buf[1024];
	int first = 1;

	while (remaining > 0) {
		size_t want = remaining < (long)sizeof(buf) ? remaining : sizeof(buf);
		ssize_t n = read(fd, buf, want);
		if (n <= 0) { close(fd); return -1; }          //peer closed early or error
		if (first && n >= 5 && strncmp(buf, "ERROR", 5) == 0) {
			close(fd);
			return -1;                                  //host rejected the request
		}
		first = 0;
		fwrite(buf, 1, n, out);
		remaining -= n;
	}

	fflush(out);
	close(fd);
	return 0;
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

char *calculate_md5(const char *filename) {
	int pipefd[2];
    pid_t pid;
    char buffer[4096];
    ssize_t bytesRead;

	if (pipe(pipefd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	
	if (pid == 0) { // Child process
		close(pipefd[0]); // Close read end of the pipe
		dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
		close(pipefd[1]); // Close the original write end of the pipe

		execlp("md5sum", "md5sum", filename, (char *)NULL);
		perror("execlp");
		exit(EXIT_FAILURE);
	} else { // Parent process
		close(pipefd[1]); // Close write end of the pipe

		bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1);
		if (bytesRead == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		buffer[bytesRead] = '\0'; // Null-terminate the string

		close(pipefd[0]); // Close read end of the pipe

		wait(NULL); // Wait for child process to finish

		char *md5 = strtok(buffer, " "); // Extract MD5 hash from output
		return md5;
	}
}

char *get_local_ip() {
	struct ifaddrs *ifaddr, *ifa;
	int family;
	char *ip = NULL;

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		// Check for IPv4 address
		if (family == AF_INET) {
			if (strcmp(ifa->ifa_name, "lo") == 0) // Skip loopback interface
				continue;
			struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
			ip = inet_ntoa(sa->sin_addr);
			break; // Stop after finding the first IPv4 address
		}
	}

	freeifaddrs(ifaddr);

	return ip;
}

void alarm_handler(int sig) {
	// This function will be called when the alarm signal is received
	printf("Alarm signal received. Performing automatic updates...\n");
	char shared_folder[256] = "./shared";
	FILE *file = fopen("serverThreadConfig.cfg", "r");
	char buffer[256];
	while (fgets(buffer, sizeof(buffer), file)) {
		char *linearg = strtok(buffer, " ");
		if (strcmp(linearg, "SHARED_FOLDER") == 0) {
			strncpy(shared_folder, strtok(NULL, " \r\n"), sizeof(shared_folder));
		}
	}
	fclose(file);
	char filepath[256];
	strcpy(filepath, shared_folder);
	strcat(filepath, "/shared.txt");
	file = fopen(filepath, "r");
	if (file == NULL) {
		printf("Error: Could not open shared.txt in %s\n", shared_folder);
		return;
	}
	while (fgets(buffer, sizeof(buffer), file)) {
		buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
		updateTracker(buffer);
	}
	fclose(file);

}

void updateTracker(const char *filename) {
	char update_req[600], ip[16], msg[1024];
	long fileSize;
	int i;
			
	//opening file to get filesize
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		printf("Error: Could not open file %s\n", filename);
		return;
	}
	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	fclose(file);

	//Format is: UPDATETRACKER <filename> <start_byte> <end_byte> <ip> <port>
	strcpy(ip, get_local_ip());
	sprintf(update_req, "updatetracker %s %d %ld %s %d\n", filename, 0, fileSize, ip, server_port);
	printf("Sending request: %s\n", update_req);
	if((write(sockid,update_req,strlen(update_req))) < 0){//inform the server of the update request
		printf("Send_request  failure\n"); exit(0);
	}

	int n; //store length of read data
    if((n = read(sockid,(char *)&msg,sizeof(msg)))< 0){// read what server has said
		printf("Read  failure\n"); exit(0); 
	}

	msg[n] = '\0'; //append null terminator to read message
    printf("%s\n", msg);
}

//write() can send fewer bytes than asked, so loop until done
static int write_all(int fd, const char *buf, size_t len) {
	size_t sent = 0;
	while (sent < len) {
		ssize_t n = write(fd, buf + sent, len - sent);
		if (n < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		sent += n;
	}
	return 0;
}

//read until /n
static int read_line(int fd, char *buf, size_t max) {
	size_t i = 0;
	while (i < max - 1) {
		char c;
		ssize_t n = read(fd, &c, 1);
		if (n <= 0) return (i == 0) ? -1 : (int)i;
		if (c == '\n') break;
		buf[i++] = c;
	}
	buf[i] = '\0';
	return (int)i;
}