#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

void checkParams(int params);
int checkSocket();
int checkPortNum(char *port);

int main(int argc, char *argv[]) {
	int sd; // Socket descriptor
	struct sockaddr_in server_address; // My address
	struct sockaddr_in from_address;  // Sender address
	char bufferReceived[1000]; // Used in recvfrom()
	int portNumber; // Provided by the user on the command line
	int rc;
	socklen_t fromLength;
	int flags = 0; // used for recvfrom
  
	// Did the user enter the correct number of parameters
	checkParams(argc);

	sd = checkSocket(); // Creates and verifies socket

	portNumber = checkPortNum(argv[1]); // Verifies and returns port number

	server_address.sin_family = AF_INET; // Use AF_INET addresses
	server_address.sin_port = htons(portNumber); // Convert port number
	server_address.sin_addr.s_addr = INADDR_ANY; // Use my IP address
  
	// Bind to address
	rc = bind(sd, (struct sockaddr *)&server_address, sizeof(struct sockaddr ));
	
	// Check for bind errors
	if (rc < 0) {
		perror("bind");
		exit (1);
	}
  
	memset(bufferReceived, 0, 1000); // Zeros out buffer

	// Giving fromLength an initial value
	fromLength = sizeof(struct sockaddr_in);
	
	// Recieve from client
	rc = recvfrom(sd, bufferReceived, 1000, flags, (struct sockaddr *)&from_address, &fromLength);

	// Check for errors
	if (rc < 0) {
		perror ("recvfrom");
		exit (1);
	}
	  
	// Print message from client
	printf ("received '%s'\n", bufferReceived);
}

void checkParams(int params) {
	if (params < 2 || params > 2) {
		printf("usage is: server <portnumber>\n");
		exit (1);
	}
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