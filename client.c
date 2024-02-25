#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

void checkParams(int params);
int checkPortNum(char *port);
int checkSocket();
char *editMessage(char *str, int size);

int main(int argc, char *argv[]) {
	int sd; // Socket descriptor
	struct sockaddr_in server_address;  // Structure for addresses
	struct sockaddr_in inaddr;  // Structure for checking addresses
	int rc;
	char bufferOut[200]; // Used in sendto()
	char serverIP[20]; // provided by the user on the command line
	int portNumber = 0; // provided by the user on the command line
	FILE *file;
	char *line = NULL;
	size_t len = 200;
	int ret;
	char fname[64];
	
	// Did the user enter the correct number of parameters
	checkParams(argc);
	
	/** This code checks to see if the ip address is a valid ip address 
	    meaning it is in dotted notation and has valid numbers **/
	if (!inet_pton(AF_INET, argv[1], &inaddr)) {
		printf("error, bad ip address\n");
		exit(1);
	}
  
	sd = checkSocket(); // Creates and verifies socket

	portNumber = checkPortNum(argv[2]); // Verifies and returns port number
  
	strcpy(serverIP, argv[1]); // Copies IP address

	server_address.sin_family = AF_INET; // Use AF_INET addresses
	server_address.sin_port = htons(portNumber); // Convert port number
	server_address.sin_addr.s_addr = inet_addr(serverIP); // Convert IP addr
	
	// Prompts the user to input a filename
	printf("Please input the file you would like read: ");
	scanf("%s", fname);
	
	// Opens file
	if((file = fopen(fname, "r")) == NULL) {
		perror("Opening file");
		exit(1);
	}
	
	// Loops until file has been fully read
	for(;;) {
		// Gets the lines of the file 
		ret = getline(&line, &len, file);
		
		// If there are no more lines end the loop
		if(ret <= 0) {
			printf("done with file\n");
			break;
		}
		
		// If line is too large end the loop
		if(sizeof(line) > 200) {
			printf("line must be <= 200 characters\n");
			break;
		}
		
		line = editMessage(line, len); // Adds ^ to the message for easier parsing
		
		memset(bufferOut, 0, 200); // Zeros out buffer
		sprintf(bufferOut, line);
		
		// Send to address
		rc = sendto(sd, bufferOut, strlen(bufferOut), 0, (struct sockaddr *) &server_address, sizeof(server_address));
		
		// Check for send errors
		if (rc < strlen(bufferOut)) {
			perror("sendto");
			exit(1);
		}
	}
	
	// Close the file and free line from memory
	fclose(file);
	if(line) {
		free(line);
	}
}

void checkParams(int params) {
	if (params < 3) {
		printf("usage is: client <ipaddr> <portnumber>\n");
		exit(1);
	}
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

char *editMessage(char* str, int size) {
	int quote = 0;
	for(int i = 0; i < size; i++) {
		// Start of the message
		if(str[i] == '"') {
			quote = 1;
		}
		// replaces spaces with ^ in message
		while(quote) {
			i++;
			if(str[i] == ' ') {
				str[i] = '^';
			// End of the message
			} else if(str[i] == '"') {
				quote = 0;
			}
		}
	}
	return str;
}