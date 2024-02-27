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
#include <stdbool.h>

#define STDIN 0
#define VERSION 6
#define TTL 3
#define MAXSEQNUMBER 100
#define MAXPARTNERS 100
// #define DEBUG

struct _partnersHost{
	char ipAddress[25];
	int portNumber;
	int location; 
	int currentSeqNumber;
	bool hasAcked[MAXSEQNUMBER];
	bool sentAck[MAXSEQNUMBER];
};

struct _partners{
	int maxHosts;
	struct _partnersHost hostInfo[MAXPARTNERS];
};

struct  _tokens {
	char key[100];
	char value [100];
};

void addFromPort(char *buffer, int myPort);
void addLocation(char *buffer, int myLocation);
void addSendPath(char *buffer, int myPort);
void addSequenceNum(char *buffer, int sequence);
void addTTL(char *buffer);
void addTypeACK(char *buffer);
void changeMSG(struct _tokens *Tokens, char *buffer, int myLocation, int tokenNum, int myPort);
void checkParams(int params);
int checkPortNum(char *port);
int checkRange(int sendLocation, int row, int col, int myLocation);
int checkTTL(char arr[][100], int index);
int checkVersion(char arr[][100], int index);
char *cleanUp(char *str);
void coordinateMap(int row, int col, int *x, int *y, int l);
char *editMessage(char *str);
int euclideanMath(int r, int c, int l1, int l2);
int getInt(int ver);
void makeBind(int *sd, char *str, struct sockaddr_in *server_address, int *myPort);
int messagePairs(char *str);
void printTable(struct _tokens *Tokens, int tokenNum);
void receiveMsg(char *buffer, int sd);
void remakeMSG(char *buffer, struct _tokens *Tokens, int tokenNum);
void sendMsg(char *buffer, int sd, struct sockaddr_in server_address);
void sendToPartners(char *buffer, int sd, struct _partners Partners, int myPort);
void switchReceiving(char *buffer, int sd, int messageIndex, char msgs[][200], int row, int col, int myPort, int myLocation, int rc, struct sockaddr_in server_address, int *sequence, struct _partners *Partners, struct _tokens *tokens);
void switchSending(char *buffer, int myLocation, int myPort, int sd, struct sockaddr_in server_address, int sequence, struct _partners *Partners);
int tokenizeBuffer(char *buffer, struct _tokens *Tokens);

void printPartners (struct _partners *Partners);
void readFile(struct _partners *Partners, int myPort, int *myLocation);
void createPartners(int sd, struct _partners *Partners, int myPort, int *myLocation);
int findSequence(int toPort, struct _partners *Partners);
char *findCharToken(struct _tokens *tokens, int tableSize, char *key);
int findIntToken(struct _tokens *tokens, int tableSize, char *key);
int findPartner(int sourcePort, struct _partners Partners);

int getSequence(char *buffer, struct _partners *Partners);

int main(int argc, char *argv[]) {
	struct _partners Partners;
	struct _tokens Tokens [100];
  	int sd; // Socket descriptor
 	int rc; // Return code
  	int myPort; // Provided by the user on the command line
  	struct sockaddr_in server_address; // My address
	fd_set socketFDS; // Socket descriptor set
	int maxSD; // How many sockets are there
	int myLocation; // Stores drone's myLocation on the grid
	char buffer[200]; // Stores the messages
	char messages[50][200]; // Stores 100 messages sent by client
	int messageIndex = 0; // Keeps track of messages that have been sent
	int sequence = 0;
	int row = 5; // Stores row number
	int col = 3; // Stores column number

	checkParams(argc);
	
	makeBind(&sd, argv[1], &server_address, &myPort);
	
	createPartners(sd, &Partners, myPort, &myLocation);

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
			switchSending(buffer, myLocation, myPort, sd, server_address, sequence, &Partners);
		}
		if (FD_ISSET(sd, &socketFDS)) {
			switchReceiving(buffer, sd, messageIndex, messages, row, col, myPort, myLocation, rc, server_address, &sequence, &Partners, Tokens);
		}
	}
}

/********************************
 **    Function Definitions    **
 ********************************/

void addFromPort(char *buffer, int myPort) {
	char fPort[20];
	memset(fPort, 0, 20); // Clears fPort
	snprintf(fPort, sizeof(fPort), " fromPort:%d", myPort); // Takes myPort and converts it into a string
	strcat(buffer, fPort); // Appends 
}

void addLocation(char *buffer, int myLocation) {
	char loc[20];
	memset(loc, 0, 20); // Clears loc
	snprintf(loc, sizeof(loc), " location:%d", myLocation); // Takes myLocation and converts it into a string
	strcat(buffer, loc); // Appends myLocation to message
}

void addSendPath(char *buffer, int myPort) {
	char path[20];
	memset(path, 0, 20); // Clears live
	snprintf(path, sizeof(path), " send-path:%d", myPort); // Makes type:ACK a string
	strcat(buffer, path); // Appends type:ACK to the message
}

void addSequenceNum(char *buffer, int sequence) {
	char num[20];
	memset(num, 0, 20); // Clears num
	snprintf(num, sizeof(num), " seqNumber:%d", sequence); // Takes sequence and converts it into a string
	strcat(buffer, num); // Appends seqNumber:sequence to the message
}

void addTTL(char *buffer) {
	char live[10];
	memset(live, 0, 10); // Clears live
	snprintf(live, sizeof(live), " TTL:%d", TTL); // Takes TTL and converts it into a string
	strcat(buffer, live); // Appends TTL:TTL to the message
}

void addTypeACK(char *buffer) {
	char type[10];
	memset(type, 0, 10); // Clears live
	snprintf(type, sizeof(type), " type:ACK"); // Makes type:ACK a string
	strcat(buffer, type); // Appends type:ACK to the message
}

void changeMSG(struct _tokens *Tokens, char *buffer, int myLocation, int tokenNum, int myPort) {
	char bigBuff[100];
	char buff[10];
	int newTTL;
	for(int i = 0; i < tokenNum; i++) {
		// Change "myLocation" value within message
		if(strcmp(Tokens[i].key, "location") == 0) {
			memset(Tokens[i].value, 0, 100);
			memset(buff, 0, 10);
			snprintf(buff, sizeof(buff), "%d", myLocation);
			strcpy(Tokens[i].value, buff);
		}
		// Decrement the TTL
		if(strcmp(Tokens[i].key, "TTL") == 0) {
			newTTL = atoi(Tokens[i].value) - 1;
			memset(Tokens[i].value, 0, 100);
			memset(buff, 0, 10);
			snprintf(buff, sizeof(buff), "%d", newTTL);
			strcpy(Tokens[i].value, buff);
		}
		// Change "send-path"
		if(strcmp(Tokens[i].key, "send-path") == 0) {
			snprintf(buff, sizeof(buff), "%d", myPort);
			if(strstr(Tokens[i].value, buff) == NULL) {
				memset(bigBuff, 0, 100);
				strcpy(bigBuff, Tokens[i].value);
				memset(Tokens[i].value, 0, 100);
				memset(buff, 0, 10);
				snprintf(buff, sizeof(buff), ",%d", myPort);
				strcat(bigBuff, buff);
				strcpy(Tokens[i].value, bigBuff);
			}
		}
	}
	remakeMSG(buffer, Tokens, tokenNum);
}

void checkParams(int params) {
	if (params < 2) {
		printf("usage is: drone6 <myPort>\n");
		exit (1);
	}
}

int checkPortNum(char *port) {
	// Is the port number a number
	for (int i=0;i<strlen(port); i++) {
		if (!isdigit(port[i])) {
			printf("The myPort isn't a number!\n");
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

int checkRange(int sendLocation, int row, int col, int myLocation) {
	int eCheck = euclideanMath(row, col, sendLocation, myLocation);
	if(myLocation > (row * col)) {
		#ifdef DEBUG
		printf("OUT OF GRID\n");
		#endif
		return -1;
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
	if(atoi(arr[index + 1]) <= 0) {
		#ifdef DEBUG
		printf("TTL is: %s", arr[index + 1]);
		#endif
		return 0;
	}
	return 1;
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

void coordinateMap(int row, int col, int *x, int *y, int l) {
	int count = 0; // Keeps track of the myLocation in the grid
	// Finds the y axis for the myLocation
	for(int j = 1; j <= row; j++) {
		// Finds the x axis for the myLocation
		for(int k = 1; k <= col; k++) {
			count++;
			// Set x and y for myLocation coordinates
			if(count == l) {
				*x = k;
				*y = j;
				break;
			}
		}
	}
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

int euclideanMath(int row, int col, int l1, int myLocation) {
	int x1, x2, y1, y2;
	coordinateMap(row, col, &x1, &y1, l1);
	coordinateMap(row, col, &x2, &y2, myLocation);

	// Applies the Euclidean distance formula to the points
	return sqrt( pow((x1 - x2), 2.0) + pow((y1 - y2), 2.0) );
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

void printTable(struct _tokens *Tokens, int tokenNum) {
	// Displays the table
	printf("**************************************************\n");
	printf("%-20s%s\n", "Name", "Value");
	for(int i = 0; i < tokenNum; i++) {
		printf("%-20s%-20s\n", Tokens[i].key, Tokens[i].value);
	}
	printf("**************************************************\n");
}

void receiveMsg(char *buffer, int sd) {
	struct sockaddr_in from_address;
	socklen_t fromLength;
	int rc, flags = 0;
	fromLength = sizeof(struct sockaddr_in); // Giving fromLength an initial value
	
	memset(buffer, 0, 200);
	// Receive from client
	rc = recvfrom(sd, buffer, 200, flags, (struct sockaddr *)&from_address, &fromLength);

	// Check for errors
	if (rc < 0) {
		perror ("recvfrom");
		exit (1);
	}
}

void remakeMSG(char *buffer, struct _tokens *Tokens, int tokenNum) {
	memset(buffer, 0, 200); // Clears the buffer
	for(int i = 0; i < tokenNum; i++) {
		// Puts a key from the table into the buffer
		strcat(buffer, Tokens[i].key);
		// Place a key value separator
		strcat(buffer, ":");
		// Place a value from the table into the buffer
		strcat(buffer, Tokens[i].value);
		// If the end of the table has not been reached
		if(i != tokenNum) {
			// Put a space to separate kv pairs
			strcat(buffer, " ");
		}
	}
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

void sendToPartners(char *buffer, int sd, struct _partners Partners, int myPort) {
	struct sockaddr_in server_address;

	for(int i = 0; i < Partners.maxHosts; i++) {
		int portNumber;
		char serverIP[20];
		memset(serverIP, 0, 20);
		portNumber = Partners.hostInfo[i].portNumber;
		strcpy(serverIP, Partners.hostInfo[i].ipAddress);

		if(portNumber != myPort) {
			server_address.sin_family = AF_INET;
    		server_address.sin_port = htons(portNumber);
    		server_address.sin_addr.s_addr = inet_addr(serverIP);
		} else {
			continue;
		}

		sendMsg(buffer, sd, server_address);
	}
	
	
	
	/* FILE *cfp;
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
		if(atoi(ptr) != myPort) {
			server_address.sin_port = htons(checkPortNum(ptr)); // Set port # in server_address
		} else {
			continue; // Do not send anything to myself
		}
		
		sendMsg(buffer, sd, server_address); // Send message
	}

	fclose(cfp); // closes config.file pointer */
}

void switchReceiving(char *buffer, int sd, int messageIndex, char msgs[][200], int row, int col, int myPort, int myLocation, int rc, struct sockaddr_in server_address, int *sequence, struct _partners *Partners, struct _tokens *Tokens) {
	int tokenNum, destinationPort, sourcePort, version, sendLocation, distance;
	memset(buffer, 0, 200); // Zeros out buffer

	receiveMsg(buffer, sd);

	sprintf(buffer, editMessage(buffer)); // Moves edited message into the buffer

	tokenNum = tokenizeBuffer(buffer, Tokens);

	destinationPort = findIntToken(Tokens, tokenNum, "toPort");
	sourcePort = findIntToken(Tokens, tokenNum, "fromPort");
	if(destinationPort == -1 || sourcePort == -1) {
		#ifdef DEBUG
			printf("No source/destination port");
		#endif
	}

	version = findIntToken(Tokens, tokenNum, "version");
	if(version == -1) {
		#ifdef DEBUG
			printf("No version number");
		#endif
	}
	if(version != VERSION) {
		#ifdef DEBUG
			printf("Wrong version number");
		#endif
	}

	sendLocation = findIntToken(Tokens, tokenNum, "location");
	if(sendLocation == -1) {
		#ifdef DEBUG
			printf("Wrong version number");
		#endif
	}
	distance = checkRange(sendLocation, row, col, myLocation);
	if(distance == -1) {
		#ifdef DEBUG
			printf("Location is not in the grid");
		#endif
	}

	if(destinationPort == myPort) {
		if(distance < 1) {
			#ifdef DEBUG
				printf("Out of range");
			#endif
		} else {
			int partnerPos = findPartner(sourcePort, *Partners);
			int seqNum = findIntToken(Tokens, tokenNum, "seqNumber");
			char *isAck = findCharToken(Tokens, tokenNum, "type");

			if(isAck != NULL && !strcmp(isAck, "ACK")) {
				#ifdef DEBUG
	    			printf ("processing an ACK\n");
				#endif
				if(partnerPos > -1) {
					if(Partners->hostInfo[partnerPos].hasAcked[seqNum] == true) {
						printf("received a dup ACK for SeqNum %d fromPort %d\n", seqNum, sourcePort);
					} else {
						printf ("received an ACK for SeqNum %d fromPort %d\n", seqNum, sourcePort);
						Partners->hostInfo[partnerPos].hasAcked[seqNum] = true;
						printTable(Tokens, tokenNum);
					}
				} else {
					printf ("received an ACK for SeqNum %d fromPort %d but couldn't find partner\n", seqNum, sourcePort);
				}
			} else {
				if(partnerPos > -1) {
					char buffer[200];
	    			memset (buffer, 0, 200);
					//changeMSG(Tokens, buffer, myLocation, tokenNum, myPort);
					sprintf (buffer, "send-path:%d TTL:%d version:%d toPort:%d fromPort:%d seqNumber:%d type:ACK location:%d", myPort, TTL, VERSION, sourcePort, myPort, seqNum, myLocation);
					sendToPartners(buffer, sd, *Partners, myPort);
					if(Partners->hostInfo[partnerPos].sentAck[seqNum] == true) {
						printf ("Duplicate packet detected! Duplication Ack being sent for ");
						printf ("seq# %d, fromPort %d\n", seqNum, sourcePort);
					} else {
						printf ("sending an ack for seqNum %d partner # %d, fromPort %d\n", seqNum, partnerPos, sourcePort);
						Partners->hostInfo[partnerPos].sentAck[seqNum] = true;
						printTable(Tokens, tokenNum);
					}
				}
			}
		}
	} else {
		int ttl;
		ttl = findIntToken(Tokens, tokenNum, "TTL");
		if(ttl == 0) {
			#ifdef DEBUG
				printf ("message received NOT for me and msg out of lives!.\n");
			#endif
		} else {
			changeMSG(Tokens, buffer, myLocation, tokenNum, myPort);
			sendToPartners(buffer, sd, *Partners, myPort);
		}
	}
}

void switchSending(char *buffer, int myLocation, int myPort, int sd, struct sockaddr_in server_address, int sequence, struct _partners *Partners) {
	memset(buffer, 0, 200); // clears buffer
	fgets(buffer, 200, stdin); // Takes input from user on command line
	
	// Adds all necessary kv pairs
	addLocation(buffer, myLocation);
	addFromPort(buffer, myPort);
	addTTL(buffer);
	addSequenceNum(buffer, getSequence(buffer, Partners));
	addSendPath(buffer, myPort);

	sendToPartners(buffer, sd, *Partners, myPort); // Sends message to all other port #'s
}

int getSequence(char *buffer, struct _partners *Partners) {
	char *ptr;
	char toPort[6];
	int copyLength;

	ptr = strstr(buffer, "toPort:");
	if (ptr == NULL) {
	    printf ("no toPort, can't send it\n");
	    exit(1);
    }

	char *ptrColon = strstr(ptr, ":");
	char *ptrEnd = strstr(ptr, " ");
	if(ptrEnd == NULL) {
		copyLength = 6;
	} else {
		copyLength = ptrColon - ptr;
	}

	strncpy(toPort, ptrColon + 1, copyLength - 1);
	int port = atoi(toPort);
	return findSequence(port, Partners);
}

int tokenizeBuffer(char *buffer, struct _tokens *Tokens) {
	int i = 0;
	char *ptr;
	ptr = strtok(buffer, ":"); // Creates the first token
	// Fills the table with key value pairs from the message
	while(ptr != NULL) {
		memset(Tokens[i].key, 0, 100);
		strcpy(Tokens[i].key, cleanUp(ptr)); // Moves the key into the table
		ptr = strtok(NULL, " "); // Makes a token of the value
		memset(Tokens[i].value, 0, 100);
		strcpy(Tokens[i].value, cleanUp(ptr)); // Moves the value into the table
		ptr = strtok(NULL, ":"); // Makes a token of the key
		i++;
	}
	return i;
}

// version:6 msg:"hi 15" toPort:47015

void makeBind(int *sd, char *str, struct sockaddr_in *server_address, int *myPort) {
	int rc;

	*sd = socket(AF_INET, SOCK_DGRAM, 0);

	if (*sd == -1) {
		printf("socket");
		exit(1);
	}

	for (int i = 0; i < strlen(str); i++) {
		if (!isdigit(str[i])) {
			printf("The port number isn't a number!\n");
			exit(1);
		}
	}

	*myPort = strtol(str, NULL, 10);

	if ((*myPort > 65535)) {
		printf("you entered an invalid socket number\n");
		exit(1);
	}

	server_address->sin_family = AF_INET; /* use AF_INET addresses */
  	server_address->sin_port = htons(*myPort); /* convert port number */
  	server_address->sin_addr.s_addr = INADDR_ANY; /* any adapter */

	// Bind to address
	rc = bind(*sd, (struct sockaddr *)server_address, sizeof(struct sockaddr));
	
	// Check for bind errors
	if (rc < 0) {
		perror("bind");
		exit (1);
	}
}

void createPartners(int sd, struct _partners *Partners, int myPort, int *myLocation) {
	memset(Partners, 0, sizeof(struct _partners));
	Partners->maxHosts = 0;
	readFile(Partners, myPort, myLocation);
	printf("Port:%d, Location:%d\n", myPort, *myLocation);
	printPartners(Partners);
}

void readFile(struct _partners *Partners, int myPort, int *myLocation) {
	FILE *cfp;
	char *ptr;
	char config[23];
	int port;
	int location;

	if((cfp = fopen("config.file", "r")) == NULL) {
		perror("Opening file");
		exit(1);
	}

	while(fgets(config, 23, cfp) != NULL) {
		ptr = strtok(config, " ");

		port = atoi(strtok(NULL, " "));

		location = atoi(strtok(NULL, "\n"));
		if(myPort == port) {
			*myLocation = location;
		}

		Partners->hostInfo[Partners->maxHosts].portNumber = port;
		Partners->hostInfo[Partners->maxHosts].location = location;
		strcpy (Partners->hostInfo[Partners->maxHosts].ipAddress, ptr);
		Partners->hostInfo[Partners->maxHosts].currentSeqNumber = 0;
		int i;
		for(i = 0; i<MAXSEQNUMBER; i++) {
			Partners->hostInfo[Partners->maxHosts].hasAcked[i] = false;     
			Partners->hostInfo[Partners->maxHosts].sentAck[i] = false;     
		}
		Partners->hostInfo[Partners->maxHosts].location = location;
		Partners->maxHosts ++;
	}

	fclose(cfp); // Closes config.file pointer
}

void printPartners(struct _partners *Partners) {
	int i = 0;

	/* the partners structure tells me how many partners/hosts i have */
	while(i < Partners->maxHosts) { // do it for each entry. 
		printf("ipaddress:%s, port:%d, location:%d, host:%d\n",
			Partners->hostInfo[i].ipAddress, //IP address
			Partners->hostInfo[i].portNumber, //port Number
			Partners->hostInfo[i].location, //location
		i);
		i++;  // increment i
	}
}

int findSequence(int toPort, struct _partners *Partners) {
	for(int i = 0; i < Partners->maxHosts; i++) {
		if(Partners->hostInfo[i].portNumber == toPort) {
			Partners->hostInfo[i].currentSeqNumber ++;
			return Partners->hostInfo[i].currentSeqNumber;
		}
	}
	return -1;
}

int findIntToken(struct _tokens *tokens, int tableSize, char *key) {
	for(int i = 0; i < tableSize; i++) {
		if(strcmp(tokens[i].key, key) == 0) {
			return atoi(tokens[i].value);
		}
	}
	return -1;
}

char *findCharToken(struct _tokens *tokens, int tableSize, char *key) {
	for(int i = 0; i < tableSize; i++) {
		if(strcmp(tokens[i].key, key) == 0) {
			return tokens[i].value;
		}
	}
	return NULL;
}

int findPartner(int sourcePort, struct _partners Partners) {
	for(int i = 0; i < Partners.maxHosts; i++) {
		if(Partners.hostInfo[i].portNumber == sourcePort) {
			return i;
		}
	}
	return -1;
}