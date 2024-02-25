#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#define STDIN 0
#define VERSION 3

void checkParams(int params);
int checkSocket();
int checkPortNum(char *port);
char *editMessage(char *str);
int messagePairs(char *str);
char *cleanUp(char *str);
void sendMsg(char *buffer, int sd, struct sockaddr_in server_address);

int main(int argc, char *argv[]) {
	int sd; // Socket descriptor
    int rc; // Return code
    int portNumber; // Provided by the user on the command line
    struct sockaddr_in server_address; // My address
    char *ptr; // Used for tokenization
    int i = 0; // Iterator variable
	fd_set socketFDS; // Socket descriptor set
  	int maxSD; // How many sockets are there
	int location;
	FILE *cfp;
	char config[23];
	char loc[14];
	struct sockaddr_in from_address;  // Sender address
	char bufferReceived[200]; // Used in recvfrom()
	socklen_t fromLength; // Length of message from client
	int flags = 0; // used for recvfrom
	int tableSize; // Size of array to hold key value pairs
	char bufferOut[200]; // Used in sendto()
	char serverIP[20]; // provided by the user on the command line

	checkParams(argc);

	// Open config.file
	if((cfp = fopen("config.file", "r")) == NULL) {
		perror("Opening file");
		exit(1);
	}

	while(fgets(config, 23, cfp) != NULL) {
		sd = checkSocket(); // Creates socket

		// Moves IP from config.file to ptr then to serverIP
		ptr = strtok(config, " ");
		strcpy(serverIP, ptr);

		ptr = strtok(NULL, " "); // Moves port # from config.file to ptr
		// Checks that the config.file port # matches ours
		if(strcmp(ptr, argv[1]) == 0) {
			portNumber = checkPortNum(ptr); // Moves ptr to portNumber

			ptr = strtok(NULL, "\n"); // Gets location # from config.file
			location = atoi(ptr); // Changes ptr to an int and moves into location

			server_address.sin_family = AF_INET; // Use AF_INET addresses
    		server_address.sin_port = htons(portNumber); // Convert port number
    		server_address.sin_addr.s_addr = inet_addr(serverIP); // Convert IP addr

			break; // If location found leave while loop
		}
	}

	fclose(cfp); // Closes config.file pointer

	printf("location:%d\n", location); // States the drones current location

	// Bind to address
	rc = bind(sd, (struct sockaddr *)&server_address, sizeof(struct sockaddr ));
	
	// Check for bind errors
	if (rc < 0) {
		perror("bind");
		exit (1);
	}

	for (;;) {
		FD_ZERO(&socketFDS);// NEW                                 
		FD_SET(sd, &socketFDS); //NEW - sets the bit for the initial sd socket
		FD_SET(STDIN, &socketFDS); // NEW tell it you will look at STDIN too
		if (STDIN > sd) // figure out what the max sd is. biggest number
			maxSD = STDIN;
		else
			maxSD = sd;
		
		rc = select(maxSD+1, &socketFDS, NULL, NULL, NULL); // NEW block until something arrives
		printf("select popped\n");
		if (FD_ISSET(STDIN, &socketFDS)){ // means i received something from the keyboard.
			
			// Open config.file
			if((cfp = fopen("config.file", "r")) == NULL) {
				perror("Opening file");
				exit(1);
			}

			memset(bufferOut, '\0', 200); // clears bufferOut
			fgets(bufferOut, sizeof(bufferOut), stdin); // Takes input from user on command line
			bufferOut[strlen(bufferOut)-1] = 0; // Removes \n from bufferOut
			snprintf(loc, sizeof(loc), " location:%d", location); // Takes the variable for location and converts it into a string
			strcat(bufferOut, loc); // Appends location to message

			// Sets port number from config.file for sending
			while(fgets(config, 23, cfp) != NULL) {
				ptr = strtok(config, " "); // Grabs IP address from config.file
				ptr = strtok(NULL, " "); // Grabs port # from config.file
				server_address.sin_port = htons(checkPortNum(ptr));

				sendMsg(bufferOut, sd, server_address); // Send message to other drones
			}

			fclose(cfp); // closes config.file pointer
		}

		if (FD_ISSET(sd, &socketFDS)){
			memset(bufferReceived, 0, 200); // Zeros out buffer

			fromLength = sizeof(struct sockaddr_in); // Giving fromLength an initial value
			
			// Recieve from client
			rc = recvfrom(sd, bufferReceived, 200, flags, (struct sockaddr *)&from_address, &fromLength);

			// Check for errors
			if (rc < 0) {
				perror ("recvfrom");
				exit (1);
			}

			sprintf(bufferReceived, editMessage(bufferReceived)); // Moves edited message into the buffer

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

			int versionCheck = 0, portCheck = 0, locationCheck = 0;
			// Checks for porper variables and outputs table if applicable
			for(i = 0; i < tableSize; i++) {
				// Check for version in message
				if(strcmp(table[i][0], "version") == 0) {
					// Checks if message version == 3
					if(atoi(table[i][1]) == VERSION) {
						versionCheck = 1;
					} else {
						printf("Skipped, wrong version\n");
					}
				}
				// Check for "port" in message
				if(strcmp(table[i][0], "port") == 0) {
					// Checks if port # in message is the same as our port #
					if(atoi(table[i][1]) == portNumber) {
						portCheck = 1;
					} else {
						printf("Received a message NOT intended for me\n");
					}
				}
				// Check for "location" in message
				if(strcmp(table[i][0], "location") == 0) {
					locationCheck = 1;
				}
				// Print table if message contains correct version, port #, and location
				if(versionCheck && portCheck && locationCheck) {
					// Displays the table
					printf("**************************************************\n");
					printf("%-20s%s", "Name", "Value\n");
					for(i = 0; i < tableSize; i++) {
						printf("%-20s%-20s\n", table[i][0], table[i][1]);
					}
					printf("%-20s%d\n", "myLocation", location);
					printf("**************************************************\n");
				}
			}
		}
	}
}

/********************************
 **    Function Definitions    **
*********************************/

void checkParams(int params) {
	if (params < 2) {
		printf("usage is: drone3 <portnumber>\n");
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

int messagePairs(char *str) {
	int pairs = 0;
	// Counts the number of key value pairs in message
	for(int i = 0; i < strlen(str); i++) {
		if(str[i] == ':') {
			pairs++;
		}
	}
	return pairs;
}

char *cleanUp(char *str) {
	int i = 0, j = 0;

	while(str[i]) {
		// Remove spaces from a word
		if (str[i] != ' ') {
			str[j++] = str[i];
		}
		// Replaces ^ with spaces for the message
		if(str[i] == '^') {
			str[i] = ' ';
		}
		i++;
	}
	str[j] = '\0';
	str[strcspn(str, "\r\n")] = 0;
	return str;
}

void sendMsg(char *buffer, int sd, struct sockaddr_in server_address) {
	int rc = 0;
	// Send to server
	rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *) &server_address, sizeof(server_address));
	
	// Check for send errors
	if (rc < strlen(buffer)) {
		perror("sendto");
		exit(1);
	}
}