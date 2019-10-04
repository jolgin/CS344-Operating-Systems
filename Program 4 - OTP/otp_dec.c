/*
 * Author: John Olgin
 * Program Name: otp_dec.c
 * Date: 8/8/19
 * Description: This program will take an encrypted string and utilize a background daemon to decrypt it.
 *		The program will then print out a readable, decrypted string to stdout. Note, this is the client,
 *		and will connect to the server daemon that will do the actual decryption.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int validateText(char plaintext[]);


int main(int argc, char *argv[]){

	// prepare variables to be used in the program
	// give them all bogus values so I know if they aren't being changed properly
	int socketFD = -1;
	int portNumber = -1;
	int charsWritten = -1;
	int charsRead = -1;

	// prepare structs to hold information regarding the connection between
	// the two processes
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;

	// ensure the correct number of arguments were provided
	if(argc != 4){
		fprintf(stderr, "Error: invalid number of arguments\n");
		exit(1);
	}



	// initialize char arrays that will hold the key, plain text and encrypted
	// text. This will be filled by file input and the recv function
	char plainString[75000];
	char key[75000];
	char cipherText[75000];
	char buffer[150000];

	// clear all the char arrays to free them of any junk values placed in them
	// upon initialization. 
	memset(plainString, '\0', sizeof(plainString));
	memset(key, '\0', sizeof(key));
	memset(cipherText, '\0', sizeof(cipherText));
	memset(buffer, '\0', sizeof(buffer));




	// open up the file provided on the command line
	// get the plain string that will be sent for decryption
	// close the file pointer
	FILE *ptr1 = fopen(argv[1], "r");
	fgets(cipherText, sizeof(cipherText), ptr1);
	fclose(ptr1);

	// validate the string doesn't contain any invalid characters
	// this is per assignment requirement
	if(validateText(cipherText) < 0){
		fprintf(stderr, "otp_dec error: input contains bad characters\n");
		exit(1);
	}

	// open up the file provided on the command line
	// get the key string generate by the keygen program to be sent to daemon
	// close the file pointer for cleanup
	FILE *ptr2 = fopen(argv[2], "r");
	fgets(key, sizeof(key), ptr2);
	fclose(ptr2);




	// if the key string is smaller in length than the encrypted text string
	// then return a text error and exit the program
	if(strlen(key) < strlen(cipherText)){
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		exit(1);
	}
	// If the key is longer than the encrypted text string, then set all the
	// unnecessary characters to null and add a newling after the last necessary
	// character
	else if(strlen(key) > strlen(cipherText)){
		key[strlen(cipherText)-1] = '\n';

		int i = 0;
		for(i = strlen(cipherText); i < sizeof key; i++){
			key[i] = '\0';
		}
	}

	

	// clear the struct of any junk values
	// set all the information required to connect to the server daemon to
	// prepare for the plain string and key to be sent for encryption
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	portNumber = atoi(argv[3]);
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNumber);
	serverHostInfo = gethostbyname("localhost");

	// Return an error if a valid host cannot be found from the info in the
	// serverHostInfo variable
	if(serverHostInfo == NULL){
		fprintf(stderr, "Client error: No host found\n");
		exit(0);
	}

	// copy over all the necessary information into the struct
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char *)serverHostInfo->h_addr, serverHostInfo->h_length);




	// check if the socket was successfully created 
	// print an error and exit if the socket isn't created 
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if(socketFD < 0){
		fprintf(stderr, "Error: socket couldn't be opened\n");
		exit(1);
	}

	// attempt connection to the accepting server to prepare for data transmission
	// if connection returns an error, print a text error and exit the program
	if(connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
		fprintf(stderr, "Error: could not contact otp_dec_d on port %d\n", portNumber);
		exit(1);
	} 
	


	// concatenate both the encrypted text and the key string into one buffer to be sent
	// this is for reliability to ensure that both strings are sent and received by the
	// server daemon.
	strcpy(buffer, cipherText);
	strcat(buffer, key);

	// send the buffer to the server encryption daemon
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);

	// call shutdown so the server's recv() loop will exit and not run forever
	// credit: https://stackoverflow.com/questions/34751399/non-terminating-while-loop-while-using-recv
	shutdown(socketFD, SHUT_WR);
	usleep(100000);


	

	// prepare the receive buffer to hold incoming bytes, clear array of junk values
	char recvBuffer[150000];
	memset(recvBuffer, '\0', sizeof(recvBuffer));

	// run a while loop to ensure the recv() gets entire string
	// credit: https://stackoverflow.com/questions/36386361/how-to-receive-big-data-with-recv-function-using-c
	while((charsRead = recv(socketFD, recvBuffer, sizeof(recvBuffer), 0)) > 0){
		// append the receive buffer to the plainString char array
		strcat(plainString, recvBuffer);

		// clear the receive buffer to prepare for another recv() call
		memset(recvBuffer, '\0', sizeof(recvBuffer));
	}
	


	// print to stdout
	fprintf(stdout, "%s", plainString);

	// close the socket for cleanup
	close(socketFD);

	return 0;
}

/*
 * Function Name: validateText()
 * Description: This function will check the encrypted text string and tell the main function
 *		whether or not it contains invalid characters. 
 * Preconditions: A char array containing the encrypted, unreadable string, must be filled and passed
 *		into this function before it's called.
 * Postconditions: The function will determine whether or not any invalid characters are contained
 * Returns: A boolean integer representing the existence or non-existence of any invalid chars
*/
int validateText(char plaintext[]){
	int i = 0;

	// iterate through each character in the plain text string
	for(i = 0; i < strlen(plaintext)-1; i++){
		// if the character has an ASCII value lower or higher than the uppercase
		// letters' ASCII values, and if the char isn't a space, then return -1 to
		// signal the existance of an invalid char
		if((plaintext[i] > 90 || plaintext[i] < 65) && plaintext[i] != ' '){
			return -1;
		}
	}

	return 0;
}