#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#define STDIN 0
#define VERSION 5
#define TTL 6
// #define DEBUG

void checkParams(int params);
int checkSocket();
int checkPortNum(char *port);
char *editMessage(char *str);
int messagePairs(char *str);
char *cleanUp(char *str);
void sendMsg(char *buffer, int sd, struct sockaddr_in server_address);
int getInt(int ver);
int euclideanMath(int r, int c, int l1, int l2);
void binding(char *sIP, int *pN, int *l, struct sockaddr_in server_address, const char *str, int socDes);
int printTable(char arr[][100], int row, int col, int portNumber, int location, int tableSize);
void tokenizeBuffer(char *buffer, char arr[][100]);
void receiveMsg(char *buffer, int sd);
void addLocation(char *buffer, int location);
void addFromPort(char *buffer, int portNumber);
void addTTL(char *buffer);
void changeMSG(char arr[][100], char *buffer, int location, int tableSize);
void remakeMSG(char *buffer, char arr[][100], int tableSize);
void sendToPartners(char *buffer, int sd, struct sockaddr_in server_address, int portNumber);
int checkVersion(char arr[][100], int index);
void coordinateMap(int row, int col, int *x, int *y, int l);
int checkRange(char arr[][100], int index, int row, int col, int location);
int checkTTL(char arr[][100], int index);
void switchSending(char *buffer, int location, int portNumber, int sd, struct sockaddr_in server_address);
void switchReceiving(char *buffer, int sd, int messageIndex, char msgs[][200], int tableSize, char tbl[][100], int row, int col, int portNumber, int location, int rc, struct sockaddr_in server_address);

int main(int argc, char *argv[]) {
  	int sd; // Socket descriptor
 	int rc; // Return code
  	int portNumber; // Provided by the user on the command line
  	struct sockaddr_in server_address; // My address
	fd_set socketFDS; // Socket descriptor set
	int maxSD; // How many sockets are there
	int location; // Stores drone's location on the grid
	char buffer[200]; // Stores the messages
	int tableSize = 0; // Size of array to hold key value pairs
	char serverIP[20]; // provided by the user on the command line
	char messages[50][200]; // Stores 100 messages sent by client
	int messageIndex = 0; // Keeps track of messages that have been sent
	char table[100][100]; // Creates the table for key value pairs
	int row = 7; // Stores row number
	int col = 6; // Stores column number

	checkParams(argc);

  	sd = checkSocket(); // Creates socket

	binding(serverIP, &portNumber, &location, server_address, argv[1], sd);

	for (;;) {
		FD_ZERO(&socketFDS);// NEW                                 
		FD_SET(sd, &socketFDS); //NEW - sets the bit for the initial sd socket
		FD_SET(STDIN, &socketFDS); // NEW tell it you will look at STDIN too
		if (STDIN > sd) // figure out what the max sd is. biggest number
			maxSD = STDIN;
		else
			maxSD = sd;
		
		rc = select(maxSD+1, &socketFDS, NULL, NULL, NULL); // NEW block until something arrives
		if (FD_ISSET(STDIN, &socketFDS)){ // Means that I received something from the keyboard.
			switchSending(buffer, location, portNumber, sd, server_address);
		}
		if (FD_ISSET(sd, &socketFDS)) {
			switchReceiving(buffer, sd, messageIndex, messages, tableSize, table, row, col, portNumber, location, rc, server_address);
		}
	}
}

/********************************
 **    Function Definitions    **
*********************************/

void checkParams(int params) {
	if (params < 2) {
		printf("usage is: drone5 <portnumber>\n");
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

int getInt(int ver) {
	char str[5];
	int num;
	// Gets the row from the user
	if(ver == 0) {
		printf("Rows: ");
	}
	// Gets the columns from the user
	if(ver == 1) {
		printf("Columns: ");
	}
	fgets(str, 3, stdin);
	num = atoi(str);

	return num;
}

int euclideanMath(int row, int col, int l1, int location) {
	int x1, x2, y1, y2;
	coordinateMap(row, col, &x1, &y1, l1);
	coordinateMap(row, col, &x2, &y2, location);

	// Applies the Euclidean distance formula to the points
	return sqrt( pow((x1 - x2), 2.0) + pow((y1 - y2), 2.0) );
}

void binding(char *serverIP, int *portNumber, int *location, struct sockaddr_in server_address, const char *str, int sd) {
	FILE *cfp;
	char config[23];
	char *ptr;
	int rc;
	// Open config.file
	if((cfp = fopen("config.file", "r")) == NULL) {
		perror("Opening file");
		exit(1);
	}

	while(fgets(config, 23, cfp) != NULL) {
		// Moves IP from config.file to ptr then to serverIP
		ptr = strtok(config, " ");
		strcpy(serverIP, ptr);

		ptr = strtok(NULL, " "); // Moves port # from config.file to ptr
		// Checks that the config.file port # matches ours
		if(strcmp(ptr, str) == 0) {
			*portNumber = checkPortNum(ptr); // Moves ptr to portNumber

			ptr = strtok(NULL, "\n"); // Gets location # from config.file
			*location = atoi(ptr); // Changes ptr to an int and moves into location

			server_address.sin_family = AF_INET; // Use AF_INET addresses
			server_address.sin_port = htons(*portNumber); // Convert port number
			server_address.sin_addr.s_addr = inet_addr(serverIP); // Convert IP addr

			break; // If location found leave while loop
		}
	}

	fclose(cfp); // Closes config.file pointer

	printf("location:%d\n", *location); // States the drones current location

	// Bind to address
	rc = bind(sd, (struct sockaddr *)&server_address, sizeof(struct sockaddr ));
	
	// Check for bind errors
	if (rc < 0) {
		perror("bind");
		exit (1);
	}
}

int printTable(char arr[][100], int row, int col, int portNumber, int location, int tableSize) {
	int vc, wr, i = 0, alive;
	// Checks for proper variables and outputs table if applicable
	for(i = 0; i < tableSize; i += 2) {
		// Check for "version" in message
		if(strcmp(arr[i], "version") == 0) {
			vc = checkVersion(arr, i); 
		}
		// Check for "location" in message
		if(strcmp(arr[i], "location") == 0) {
			wr = checkRange(arr, i, row, col, location);
		}
		if(strcmp(arr[i], "TTL") == 0) {
			alive = checkTTL(arr, i);
		}
		if((i + 2) == tableSize) {
			// Print table if message contains correct version, port #, and location
			if(vc && wr && alive) {
				// Displays the table
				printf("**************************************************\n");
				printf("%-20s%s\n", "Name", "Value");
				for(i = 0; i < tableSize; i += 2) {
					printf("%-20s%-20s\n", arr[i], arr[i + 1]);
				}
				printf("**************************************************\n");
			}
		}
	}
	return alive;
}

void tokenizeBuffer(char *buffer, char arr[][100]) {
	int i = 0;
	char *ptr;
	ptr = strtok(buffer, ":"); // Creates the first token
	// Fills the table with key value pairs from the message
	while(ptr != NULL) {
		memset(arr[i], 0, 100);
		strcpy(arr[i], cleanUp(ptr)); // Moves the key into the table
		ptr = strtok(NULL, " "); // Makes a token of the value
		i++;
		memset(arr[i], 0, 100);
		strcpy(arr[i], cleanUp(ptr)); // Moves the value into the table
		ptr = strtok(NULL, ":"); // Makes a token of the key
		i++;
	}
}

void receiveMsg(char *buffer, int sd) {
	struct sockaddr_in from_address;
	socklen_t fromLength;
	int rc, flags = 0;
	fromLength = sizeof(struct sockaddr_in); // Giving fromLength an initial value
			
	// Receive from client
	rc = recvfrom(sd, buffer, 200, flags, (struct sockaddr *)&from_address, &fromLength);

	// Check for errors
	if (rc < 0) {
		perror ("recvfrom");
		exit (1);
	}
}

void addLocation(char *buffer, int location) {
	char loc[20];
	memset(loc, 0, 20); // Clears loc
	snprintf(loc, sizeof(loc), " location:%d", location); // Takes location and converts it into a string
	strcat(buffer, loc); // Appends location to message
}

void addFromPort(char *buffer, int portNumber) {
	char fPort[20];
	memset(fPort, 0, 20); // Clears fPort
	snprintf(fPort, sizeof(fPort), " fromPort:%d", portNumber); // Takes portNumber and converts it into a string
	strcat(buffer, fPort); // Appends 
}

void addTTL(char *buffer) {
	char live[10];
	memset(live, 0, 10); // Clears live
	snprintf(live, sizeof(live), " TTL:%d", TTL); // Takes TTL and converts it into a string
	strcat(buffer, live); // Appends TTL:TTL to the message
}

void changeMSG(char arr[][100], char *buffer, int location, int tableSize) {
	char loc[5];
	int newTTL;
	for(int i = 0; i < tableSize; i += 2) {
		// Change "location" value within message
		if(strcmp(arr[i], "location") == 0) {
			memset(arr[i + 1], 0, 100);
			snprintf(loc, sizeof(loc), "%d", location);
			strcpy(arr[i + 1], loc);
		}
		// Decrement the TTL
		if(strcmp(arr[i], "TTL") == 0) {
			newTTL = atoi(arr[i + 1]) - 1;
			memset(arr[i + 1], 0, 100);
			memset(loc, 0, 5);
			snprintf(loc, sizeof(loc), "%d", newTTL);
			strcpy(arr[i + 1], loc);
		}
	}

	remakeMSG(buffer, arr, tableSize);
}

void remakeMSG(char *buffer, char arr[][100], int tableSize) {
	memset(buffer, 0, 200); // Clears the buffer
	for(int i = 0; i < tableSize; i += 2) {
		// Puts a key from the table into the buffer
		strcat(buffer, arr[i]);
		// Place a key value separator
		strcat(buffer, ":");
		// Place a value from the table into the buffer
		strcat(buffer, arr[i + 1]);
		// If the end of the table has not been reached
		if((i + 1) != tableSize) {
			// Put a space to separate kv pairs
			strcat(buffer, " ");
		}
	}
}

void sendToPartners(char *buffer, int sd, struct sockaddr_in server_address, int portNumber) {
	FILE *cfp;
	char *ptr;
	char config[23];

	// Open config.file
	if((cfp = fopen("config.file", "r")) == NULL) {
		perror("Opening file");
		exit(1);
	}

	// Sets port number from config.file for sending
	while(fgets(config, 23, cfp) != NULL) {
		server_address.sin_family = AF_INET; // Use AF_INET addresses
		ptr = strtok(config, " "); // Grabs IP address from config.file
		server_address.sin_addr.s_addr = inet_addr(ptr); // Convert IP addr
		ptr = strtok(NULL, " "); // Grabs port # from config.file
		// If the port # is not the same as mine
		if(atoi(ptr) != portNumber) {
			server_address.sin_port = htons(checkPortNum(ptr)); // Set port # in server_address
		} else {
			continue; // Do not send anything to myself
		}
		
		sendMsg(buffer, sd, server_address); // Send message
	}

	fclose(cfp); // closes config.file pointer
}

int checkVersion(char arr[][100], int index) {
	// Checks if message value for version == VERSION
	if(atoi(arr[index + 1]) == VERSION) {
		return 1;
	} else {
		#ifdef DEBUG
		printf("Wrong version number\n");
		#endif
		return 0;
	}
}

void coordinateMap(int row, int col, int *x, int *y, int l) {
	int count = 0; // Keeps track of the location in the grid
	// Finds the y axis for the location
	for(int j = 1; j <= row; j++) {
		// Finds the x axis for the location
		for(int k = 1; k <= col; k++) {
			count++;
			// Set x and y for location coordinates
			if(count == l) {
				*x = k;
				*y = j;
				break;
			}
		}
	}
}

int checkRange(char arr[][100], int index, int row, int col, int location) {
	int eCheck = euclideanMath(row, col, atoi(arr[index + 1]), location);
	if(location > (row * col)) {
		#ifdef DEBUG
		printf("OUT OF GRID\n");
		#endif
		return 0;
	} else if(eCheck > 2) {
		#ifdef DEBUG
		printf("NOT IN RANGE\n");
		#endif
		return 0;
	} else if(eCheck > 0) {
		#ifdef DEBUG
		printf("IN RANGE\n");
		#endif
		return 1;
	}
	return 0;
}

int checkTTL(char arr[][100], int index) {
	if(atoi(arr[index + 1]) < 0) {
		#ifdef DEBUG
		printf("TTL is: %s", arr[index + 1]);
		#endif
		return 0;
	}
	return 1;
}

void switchSending(char *buffer, int location, int portNumber, int sd, struct sockaddr_in server_address) {
	memset(buffer, 0, 200); // clears buffer
	fgets(buffer, 200, stdin); // Takes input from user on command line
	// Adds all necessary kv pairs
	addLocation(buffer, location);
	addFromPort(buffer, portNumber);
	addTTL(buffer);

	sendToPartners(buffer, sd, server_address, portNumber); // Sends message to all other port #'s
}

void switchReceiving(char *buffer, int sd, int messageIndex, char msgs[][200], int tableSize, char tbl[][100], int row, int col, int portNumber, int location, int rc, struct sockaddr_in server_address) {
	memset(buffer, 0, 200); // Zeros out buffer

	receiveMsg(buffer, sd);

	sprintf(buffer, editMessage(buffer)); // Moves edited message into the buffer

	tableSize = messagePairs(buffer) * 2; // Sets tableSize

	tokenizeBuffer(buffer, tbl);

	rc = printTable(tbl, row, col, portNumber, location, tableSize);

	if(rc >= 0) {
		memset(msgs[messageIndex], 0, 200); // Clears the message at the messageIndex
		strcpy(msgs[messageIndex], buffer); // Moves the buffer string into the messages array

		changeMSG(tbl, buffer, location, tableSize);

		sendToPartners(buffer, sd, server_address, portNumber);

		messageIndex = (messageIndex + 1) % 50;
	}
}