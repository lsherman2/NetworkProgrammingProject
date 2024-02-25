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


int main(int argc, char *argv[]) {
	int sd; // Socket descriptor
	struct sockaddr_in server_address;  // Structure for addresses
	struct sockaddr_in inaddr;  // Structure for checking addresses
	int rc;
	char bufferOut[100]; // Used in sendto()
	char serverIP[20]; // provided by the user on the command line
	int portNumber = 0; // provided by the user on the command line
	
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
  
	memset(bufferOut, 0, 100); // Zeros out buffer
	sprintf(bufferOut, "hello world");
	
	// Send to address
	rc = sendto(sd, bufferOut, strlen(bufferOut), 0, (struct sockaddr *) &server_address, sizeof(server_address));
	
	// Check for send errors
	if (rc < strlen(bufferOut)) {
		perror("sendto");
		exit(1);
	}
}

void checkParams(int params) {
	if (params < 3 || params > 3) {
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