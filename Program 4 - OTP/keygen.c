/*
 * Author: John Olgin
 * Program Name: keygen.c
 * Date: 8/8/2019
 * Description: This program will generate an key of a specific length to be used in the encryption and
 *	decription of plaintext strings.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]){
	srand(time(NULL));

	int length = -1;
	int randIndex = -5;

	// ensure the correct number of arguments are provided on the command line
	if (argc < 2){
		fprintf(stderr, "Error: invalid number of arguments\n");
		exit(1);
	}

	// convert the char length into an integer
	length = atoi(argv[1]);


	// Dynamically create the buffer that will hold the randomly generated key string
	// This ensures the char array pointed to will be the exact length given on the command line
	char *buffer = malloc(sizeof(char) * length);

	// Initialize the array of available chars so the function has a list to index from
	char list[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	int i = 0;

	// Fill each element in the key string with a randomly generated char
	// This is the string that will be printed to stdout for other programs to use
	for(i = 0; i < length; i++){
		randIndex = rand() % 27;
		buffer[i] = list[randIndex];
	}

	// print to stdout
	fprintf(stdout, "%s\n", buffer);

	free(buffer);
	return 0;
}