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
int messagePairs(char *str);

int main(int argc, char *argv[]) {
	int sd; // Socket descriptor
	struct sockaddr_in server_address; // My address
	struct sockaddr_in from_address;  // Sender address
	char bufferReceived[1000]; // Used in recvfrom()
	int portNumber; // Provided by the user on the command line
	int rc; // Return code
	socklen_t fromLength; // Length of message from client
	int flags = 0; // used for recvfrom
	char *ptr; // Used to hold tokens from bufferReceived
	int tableSize; // Size of array to hold key value pairs
	int i = 0; // Iterator variable
  
	checkParams(argc); // Checks the parameters

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

		fromLength = sizeof(struct sockaddr_in); // Giving fromLength an initial value
	
		// Recieve from client
		rc = recvfrom(sd, bufferReceived, 1000, flags, (struct sockaddr *)&from_address, &fromLength);

		// Check for errors
		if (rc < 0) {
			perror ("recvfrom");
			exit (1);
		}

		tableSize = messagePairs(bufferReceived); // Sets tableSize
		char *table[tableSize][2]; // Creates the table for key value pairs
	
	
		i = 0;
		ptr = strtok(bufferReceived, ":"); // Creates the first token
		// Fills the table with key value pairs from the message
		while(ptr != NULL) {
			table[i][0] = cleanUp(ptr); // Moves the key into the table
			ptr = strtok(NULL, " "); // Makes a token of the value
			table[i][1] = cleanUp(ptr); // Moves the value into the table
			ptr = strtok(NULL, ":"); // Makes a token of the key
			i++;
		}

		// Prints the key value pairs
		for(i = 0; i < tableSize; i++) {
			// Look for the key "port"
			if(strcmp(table[i][0], "port") == 0) {
				// If the value matches portNumber
				if(atoi(table[i][1]) == portNumber) {
					// Displays the table
					printf("**************************************************\n");
					printf("%-20s%s", "Name", "Value\n");
					for(i = 0; i < tableSize; i++) {
						printf("%-20s%-20s\n", table[i][0], table[i][1]);
					}
					printf("**************************************************\n");
				} else {
					printf("Received a message for port %s\n", table[i][1]);
				}
			}
		}
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
	if ((pNum > 65535)) {
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

int messagePairs(char *str) {
	int pairs = 0;
	for(int i = 0; i < strlen(str); i++) {
		if(str[i] == ':') {
			pairs++;
		}
	}
	return pairs;
}