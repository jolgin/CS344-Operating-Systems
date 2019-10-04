/*
 * Author: John Olgin
 * Program Name: otp_dec_d.c
 * Date: 8/8/19
 * Description: This program will operate as a decryption daemon. It is a tool that will be used by the 
 *	otp_dec.c program. It will receive a string of unreadable, encrypted text, along with an encryption key, and encrypt
 *	and return the decrypted string back to the client. 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void decrypt(char key1[], char fileText[], char encryptText[]);
int convertToInt(char letter);
char converToChar(int index);
void checkTerminatedProcesses(int exitMethod);


int main(int argc, char *argv[]){

	// prepare variables to be used in the program
	// give them all bogus values so I know if they aren't being changed properly
	int listenSocketFD = -1;
	int estabSocketFD = -1;
	int portNumber = -1;
	int charsRead = -1;

	// prepare structs to hold information regarding the connection between
	// the two processes
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;




	// Ensure the correct number of arguments were provided
	if(argc != 2){
		fprintf(stderr, "Incorrect number of arguments\n");
		exit(1);
	}



	// set all the server address variables to be used in the connection
	// clear the struct first to ensure that it's truly empty
	memset((char *)&serverAddress, '\0', sizeof(serverAddress));
	portNumber = atoi(argv[1]);
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNumber);
	serverAddress.sin_addr.s_addr = INADDR_ANY;




	// set up the listen socket to listen for incoming client connections
	// also, check if the socket was properly initialized
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSocketFD < 0){
		error("Error: socket creation failed\n");
	}

	// bind the socket and ensure that the socket was successfully bound
	if(bind(listenSocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
		error("Error: binding failed\n");
	}

	// start listening on the socket to prepare for incoming connections
	listen(listenSocketFD, 5);




	// start primary loop to accept connections
	while(1){
		// initialize char arrays that will hold the key, plain text and encrypted
		// text. This will be filled by file input and the encryption function
		char key[75000];
		char plaintext[75000];
		char buffer[150000];
		char cipherText[75000];
		int exitMode = -5;

		// clear all the char arrays to free them of any junk values placed in them
		// upon initialization. 
		memset(key, '\0', sizeof(key));
		memset(plaintext, '\0', sizeof(plaintext));
		memset(buffer, '\0', sizeof(buffer));
		memset(cipherText, '\0', sizeof(cipherText));




		// check for any child processes that have ended
		checkTerminatedProcesses(exitMode);

		// save the size of the struct holding the client address
		sizeOfClientInfo = sizeof(clientAddress);

		// accept any incoming connections from clients
		estabSocketFD = accept(listenSocketFD, (struct sockaddr*)&clientAddress, &sizeOfClientInfo);

		// check that a client connection was properly accepted 
		// Do this prior to any data transmission to ensure stability
		if(estabSocketFD < 0){
			fprintf(stderr, "Error: error on accept\n");
		}




		// Only fork() child if a proper connection is accepted by the server
		if(estabSocketFD >= 0){

			// start a new process to do the actual decryption
			int childID = fork();

			// prepare the receive buffer to hold incoming bytes, clear array of junk values
			char recvBuffer[150000];
			memset(recvBuffer, '\0', sizeof(recvBuffer));



			switch(childID){
				// return  error if a process isn't spawned correctly and exit
				case -1:
					perror("Error: failed to spawn process");
					exit(1);
					break;

				// start of the child process code
				case 0:
					// run a while loop to ensure the recv() gets entire string
					// credit: https://stackoverflow.com/questions/36386361/how-to-receive-big-data-with-recv-function-using-c
					while((charsRead = recv(estabSocketFD, recvBuffer, sizeof(recvBuffer), 0)) > 0){
						// append the receive buffer to the buffer char array
						strcat(buffer, recvBuffer);

						// clear the receive buffer to prepare for another recv() call
						memset(recvBuffer, '\0', sizeof(recvBuffer));
					}



					// Break up the buffer into the plaintext and the key and place
					// them into their own char arrays to be used by the encryption function
					char *ptr = strtok(buffer, "\n");
					strcpy(cipherText, ptr);

					ptr = strtok(NULL, "\n");
					if(ptr != NULL){
						strcpy(key, ptr);
					}

					// enter newlines into the plain text and key char arrays
					// This helps with the encryption and printing of the strings
					cipherText[strlen(cipherText)] = '\n';
					key[strlen(key)] = '\n';




					// run the decryption algorithm
					// return the encrypted string back to the client
					decrypt(key, plaintext, cipherText);
					send(estabSocketFD, plaintext, sizeof(plaintext), 0);

					// call shutdown so the client's recv() loop will exit and not run forever
					// credit: https://stackoverflow.com/questions/34751399/non-terminating-while-loop-while-using-recv
					shutdown(estabSocketFD, SHUT_WR);

					// close the socket for good cleanup
					close(estabSocketFD);

					// exit the child process
					exit(0);

				// this is the parent process code
				default:
					// check if the child process has terminated yet, don't wait
					checkTerminatedProcesses(exitMode);

			}
		}
	}

	return 0;
}


/*
 * Function Name: decrypt()
 * Description: This function takes an unreadable text string and decrypts it using a key generated
 *		by the keygen program. 
 * Preconditions: This function requires that the main function successfully receives an encryption
 *		key and encrypted text string to be decrypted from the client
 * Postconditions: A char array containing an decrypted version of the plain text will be filled
 *		and accessible by the main program
 * Returns: none
*/
void decrypt(char key1[], char fileText[], char encryptText[]){
	int i = 0;

	// read through every character except for the ending newline character
	for(i = 0; i < strlen(encryptText)-1; i++){

		// convert each character from ciphertext and key to an integer to prepare for use 
		// in the decryption equation
		int cipher = convertToInt(encryptText[i]);
		fflush(stdout);
		int key = convertToInt(key1[i]);


		// convert the cipher text and key integers to a newly decrypted integer and
		// convert it to the appropriate character
		int plain = (cipher - key) % 27;

		// if the plain text integer is negative, add 27 to get the character integer
		if(plain < 0){
			plain += 27;
		}

		// convert it to the appropriate plain text char and add it to the plain text
		// char array
		fileText[i] = converToChar(plain);
	}

	// add a newline to the end of the decrypted  text string
	fileText[i] = '\n';
}


/*
 * Function Name: convertToInt()
 * Description: This will convert the character passed in to an appropriate integer depending
 * 		on the character's position in the char array of available characters.
 * Preconditions: A valid character must be passed in to the function.
 * Postconditions: An integer will be generated based on the character passed in.
 * Returns: an integer representing the character's position in the array of available chars
*/ 
int convertToInt(char letter){
	// Initialize the array that holds all the available characters
	char list[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	int i = 0;

	// Iterate through the char array of available chars, return the index of the 
	// position where a match is found with the char passed in
	for(i = 0; i < 27; i++){
		if(letter == list[i]){
			return i;
		}
	}

	// return negative 1 if no match was found (should not happen)
	return -1;
}


/*
 * Function Name: convertToChar()
 * Description: This will convert the integer passed in to an appropriate character depending
 * 		on the character's position in the char array of available characters.
 * Preconditions: A valid integer must be passed in to the function.
 * Postconditions: A character will be generated based on the integer passed in.
 * Returns: a char matching the character's position in the array of available chars
*/ 
char converToChar(int index){
	// Initialize the array that holds all the available characters
	char list[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	// Iterate through the char array of available chars, then return the char in the
	//  position of the index passed in from the encryption function
	// return -1 if a match isn't found (should not happen)
	if(index >= 0 && index <= 26){
		return list[index];
	}
	else{
		return -1;
	}
}


/*
 * Function Name: checkTerminatedProcesses()
 * Description: This function will periodically check if any child processes have
 *		terminated.
 *	Preconditions: An integer for the exit status must be created and passed in to be changed.
 *	Postconditions: The exit status will be updated and changed depending if any process has
 *		actually terminated. All of this is silent and nothing will be printed to screen. Processes
 		will just be waited for.
 *	Returns: none
*/
void checkTerminatedProcesses(int exitMethod){
	usleep(50000);

	// check for any process that has recently terminated
	int exitPID = waitpid(-1, &exitMethod, WNOHANG);

	// continue to wait for terminating processes as long as they are foun
	while(exitPID > 0){
		exitPID = waitpid(-1, &exitMethod, WNOHANG);
	}
}
