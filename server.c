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
char *cleanUp(char *str);

int main(int argc, char *argv[]) {
	int sd; // Socket descriptor
	struct sockaddr_in server_address; // My address
	struct sockaddr_in from_address;  // Sender address
	char bufferReceived[1000]; // Used in recvfrom()
	int portNumber; // Provided by the user on the command line
	int rc;
	socklen_t fromLength;
	int flags = 0; // used for recvfrom
	char *ptr;
	char *table[15][2];
  
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

	while(1) {
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
	
	// Creates a table of information recieved
	int i=0;
	ptr = strtok(bufferReceived, ":");
	while(ptr != NULL) {
		table[i][0] = cleanUp(ptr);
		ptr = strtok(NULL, " ");
		table[i][1] = cleanUp(ptr);
		ptr = strtok(NULL, ":");
		i++;
	}
	
	// Displays the table
	printf("**************************************************\n");
	printf("%-15s%s", "Name", "Value\n");
	for(i = 0; i < 15; i++) {
		printf("%-15s%-15s\n", table[i][0], table[i][1]);
	}
	printf("**************************************************\n");
	
	}
}

void checkParams(int params) {
	if (params < 2) {
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

char *cleanUp(char *str) {
	int i = 0, j = 0;
	while(str[i]) {
		if (str[i] != ' ') {
			str[j++] = str[i];
		}
		if(str[i] == '^') {
			str[i] = ' ';
		}
		i++;
	}
	str[j] = '\0';
	str[strcspn(str, "\r\n")] = 0;
	return str;
}