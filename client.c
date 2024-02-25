#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

// void checkParams(int params);
int checkPortNum(char *port);
int checkSocket();
char *editMessage(char *str);

int main(int argc, char *argv[]) {
	int sd; // Socket descriptor
	struct sockaddr_in server_address; // Structure for addresses
	struct sockaddr_in inaddr; // Structure for checking addresses
	int rc; // Return code
	char bufferOut[200]; // Used in sendto()
	char serverIP[20]; // provided by the user on the command line
	int portNumber = 0; // provided by the user on the command line
	FILE *mfp; // Message file pointer
	FILE *cfp; // Config file pointer
	char message[200]; // Message file line
	char config[21]; // Config file line
	char *ptr; // Holds token for ip address and port number

	// Opens file
	if((cfp = fopen("config.file", "r")) == NULL) { 
		perror("Opening file");
		exit(1);
	}
	
	while(fgets(config, 21, cfp) != NULL) {
		ptr = strtok(config, " "); // contains the ip address from the config file

		/** This code checks to see if the ip address is a valid ip address 
		meaning it is in dotted notation and has valid numbers **/
		if (!inet_pton(AF_INET, ptr, &inaddr)) {
			printf("error, bad ip address\n");
			exit(1);
		}
	
		sd = checkSocket(); // Creates and verifies socket

		strcpy(serverIP, ptr); // Copies IP address

		ptr = strtok(NULL, "\n"); // contains the port number form the config file
		portNumber = checkPortNum(ptr); // Verifies and returns port number
	
		server_address.sin_family = AF_INET; // Use AF_INET addresses
		server_address.sin_port = htons(portNumber); // Convert port number
		server_address.sin_addr.s_addr = inet_addr(serverIP); // Convert IP addr
		
		// Opens file
		if((mfp = fopen("messages.txt", "r")) == NULL) {
			perror("Opening file");
			exit(1);
		}
		
		// Loops until file has been fully read
		while(fgets(message, 200, mfp)) {
			
			// If line is too large end the loop
			if(strlen(message) > 200) {
				printf("line must be <= 200 characters\n");
				break;
			}
			
			memset(bufferOut, 0, 200); // Zeros out buffer
			sprintf(bufferOut, editMessage(message)); // Moves edited message into the buffer

			// Send to address
			rc = sendto(sd, bufferOut, strlen(bufferOut), 0, (struct sockaddr *) &server_address, sizeof(server_address));
			
			// Check for send errors
			if (rc < strlen(bufferOut)) {
				perror("sendto");
				exit(1);
			}
		}
		// Close the message file
		fclose(mfp);
	}
	// Close the confige file
	fclose(cfp);
}

int checkPortNum(char *port) {
	// Is the port number a number
	for (int i=0;i<strlen(port); i++) {
		if (!isdigit(port[i])) {
			printf("The Portnumber isn't a number!\n");
			exit(1);
		}
	}
	
	int pNum = strtol(port, NULL, 10);
  
	// Is port number in correct range
	if ((pNum > 65535) || (pNum < 0)) {
		printf("you entered an invalid socket number\n");
		exit(1);
	}
	
	return pNum;
}

int checkSocket() {
	int sockOut = socket(AF_INET, SOCK_DGRAM, 0); // Creates a socket
	
	// Check for socket errors
	if (sockOut == -1) {
		printf("socket");
		exit(1);
	}
	
	return sockOut;
}

char *editMessage(char* str) {
	int quote = 0;
	for(int i = 0; i < strlen(str); i++) {
		// Finds the start of the message
		if(str[i] == '"') {
			quote = 1;
		}
		// replaces spaces with ^ in message
		while(quote) {
			i++;
			if(str[i] == ' ') {
				str[i] = '^';
			// Finds the end of the message
			} else if(str[i] == '"') {
				quote = 0;
			}
		}
	}
	return str;
}