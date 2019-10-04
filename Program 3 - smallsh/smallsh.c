/*
 *	Name: John Olgin
 *	Program Name: smallsh
 *	Description: This program will attemp to implement a small, simple bash shell. It will support
 *		a few built in commands, but will utilize new processes to execute most available bash commands.
 *	Date: July 29, 2019
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>

#define MAX_LENGTH 2049
int foregroundOnlyMode = 0;
int termPID = -5;

void getUserInput(char userInput[]);
int parseInput(char userInput[], char *argumentList[]);
int checkForBuiltIn(char *argumentList[]);
int checkInputRedirect(char *argumentList[], int totalArguments);
int checkOutputRedirect(char *argumentList[], int totalArguments);
int checkBackground(char *argumentList[], int totalArguments);
int updateArgList(char *argumentList[], char *newArgList[], int inRedirect, int outRedirect, int totArgs);
int checkStatus(int exitMethod);
void replace(char *args[], int argPosition, int parentPID);
void checkTerminatedProcesses(int exitMethod);
void catchSIGINT(int signo);
void catchStop(int signo);


int main(){

	// initialize char array and arrays of pointers that will hold user input,
	// the raw list of arguments, and the list of arguments excluding symbols and
	// redirection files
	char input[MAX_LENGTH];
	char *commands[512];
	char *newCommands[512];


	// prepare variable that holds exit signals from child processes
	int childExitMethod = -5;

	// save the shell pid for use in other program functions
	int shellPID = getpid();


	// This section is taken almost verbatim the lecture slides
	// initialize two sigaction structs so we can tell them exactly what to do 
	// when certain signals are used.
	struct sigaction action = {0};
    struct sigaction action2 = {0};
    struct sigaction action3 = {0};
    
    // Tell main shell to ignore any SIGINT signal from the keyboard
    // use SA_RESTART to re-enter after the signal handler is done
    action.sa_handler = catchSIGINT;
    action.sa_flags = SA_RESTART;          
    sigfillset(&(action.sa_mask));
    sigaction(SIGINT, &action, NULL);
    

    // Tell the parent process to use the catchStop() function when a SIGTSTP
    // signal is used on the keyboard. Use SA_RESTART to re-enter after the signal
    // handler. 
    memset(&action2, 0, sizeof(action2));
    action2.sa_handler = catchStop;
    action2.sa_flags = SA_RESTART;
    sigfillset(&(action2.sa_mask));
    sigaction(SIGTSTP, &action2, NULL);
    // --- end of code used from lectures



    // Loop until receiving the exit command
	int run = 1;
	while(run == 1){

		fflush(stdout);
		termPID = -5;
		// reset the arrays of pointers back to zero to prepare for new input
		memset(commands, 0, 512);
		memset(newCommands, 0, 512);

		// check for any processes that may have terminated while running
		checkTerminatedProcesses(childExitMethod);

		// get raw input and parse it into individual strings, while saving the
		// total number arguments in the argument list
		getUserInput(input);
		int totalArgs = parseInput(input, commands);

		if(commands[0] == NULL){
			continue;
		}
		// This will check for built-in commands, redirection commands, and background
		// commands. This section will save the position of any redirection or background
		// commands to be used later. It will also return which built-in command is used 
		// if any.
		int command = checkForBuiltIn(commands);
		int inputRedirect = checkInputRedirect(commands, totalArgs);
		int outputRedirect = checkOutputRedirect(commands, totalArgs);
		int background = checkBackground(commands, totalArgs);


		// If the echo command is followed by nothing, then simply re-prompt the user for
		// another command. Do the same if the there's a preceding "#" on the command line
		if((strcmp(commands[0], "echo") == 0 && commands[1] == 0) || strstr(commands[0], "#") != NULL){
			printf("\n");
			fflush(stdout);
			continue;
		}

		// This section runs only if no built-in commands were provided on the command line
		else if(command == -1){

			// fork a new child and save the pid
			// the following switch statement is credited to the lecture materials as it
			// follows along very closely
			int childPID = fork();
			switch(childPID){
				case -1:
					// In the case that fork() returns -1 (error), state this and exit
					perror("Error creating child process");
					exit(1);
					break;


				// Case 0 covers a successfully created fork to be used to execute commands
				case 0:
					// If running as a foreground process, then we want it to terminate itself
					// upon receipt of a SIGINT signal from the keyboard. This section implements
					// that functionality per the assignment requirements.
					if(background == -1){
						action.sa_handler = SIG_DFL;
						action.sa_flags = 0; 
    					sigfillset(&(action.sa_mask));
						sigaction(SIGINT, &action, NULL);
					}


					// if input redirection is commanded by the user
					if(inputRedirect != -1){

						// Attempt to open a read only filename provided by the user 
						// in the command list and flush()
						int inputFD = open(commands[inputRedirect+1], O_RDONLY);
						fflush(stdout);


						// Exit and display a message if there's an error with the file
						if(inputFD < 0){
							printf("cannot open %s for input\n", commands[inputRedirect+1]);
							fflush(stdout);
							exit(1);
						} 
						// If the file is opened correctly, redirect stdin to come from
						// this newly opened file and close the original file descriptor
						else {
							dup2(inputFD, STDIN_FILENO);
							close(inputFD);
						}
					}
					// In other words, foreground only mode is off and the user wants 
					// to run a background process with no redirected input, so by default we redirect
					// to /dev/null
					else if(inputRedirect == -1 && background != -1 && foregroundOnlyMode != 1){
						int source = open("/dev/null", O_RDONLY);
						dup2(source, 0);
					}


					// if output redirection is commanded by the user
					if(outputRedirect != -1){
						// get a file descriptor for the new file to pass into dup2() and flush
						int outputFD = open(commands[outputRedirect+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
						fflush(stdout);

						// Exit and display a message if there's an error opening the file
						if(outputFD < 0){
							perror("Error opening output file!\n");
							exit(1);
						} 
						// If opened correctly, redirect stdout to come from newly opened file and
						// close original file descriptor
						else{
							dup2(outputFD, STDOUT_FILENO);
							close(outputFD);
						}
					}
					// In other words, foreground only mode is off and the user wants 
					// to run a background process with no redirected output, so by default we redirect
					// to /dev/null
					else if(outputRedirect == -1 && background != -1 && foregroundOnlyMode != 1){
						int destination = open("/dev/null", O_WRONLY);
						dup2(destination, 1);
					}

					// remove all redirection symbols, redirections files, and background symbols
					int newSize = updateArgList(commands, newCommands, inputRedirect, outputRedirect, totalArgs);


					// iterate through the entire argument list
					// if there's an instance of "$$", call the replace() function to remove
					// it and replace it with the process id of the shell
					int k = 0;
					for(k = 0; k < newSize; k++){
						if(strstr(newCommands[k], "$$") != NULL){
							replace(newCommands, k, shellPID);
						}
					}

					// execute the command and display message and error status if execution fails
					if(execvp(commands[0], newCommands) < 0){
						printf("%s: no such file or directory\n", commands[0]);
						fflush(stdout);
						exit(1);
					}


				// This section is only executed by the parent process since the child process will
				// resolve in either the Case -1 or Case 0 sections of the switch statement	
				default:

					// If the "&" is entered and fg only mode is off, then display a message
					// stating the process id of the child and don't wait for its termination
					// This is because we are required to immediately return command line control
					// to the user
					if(background != -1 && foregroundOnlyMode == 0){
						printf("background pid is %d\n", childPID);
						fflush(stdout);
					} 
					// If no "&" was provided, wait for the child process to resolve before
					// returning command line control to the user
					else{
						waitpid(childPID, &childExitMethod, 0);
					}
					break;
			}
		}

		// This section runs if any of the 3 built-in commands are provided. This is because
		// the parent should be executing these directly without forking another process
		else{
			int childStatus = 0;

			// Check for any instance of "$$", if it exists then call replace() to replace the
			// instance with the shell's process id. This is "$$" expansion
			int k = 0;
			for(k = 0; k < totalArgs; k++){
				if(strstr(commands[k], "$$") != NULL){
					replace(commands, k, shellPID);
				}
			}

			// Run a case depending on which built-in command was given on the command line
			switch(command){
				// if the "exit" command was given, call exit(0)
				case 1:
					fflush(stdout);
					exit(0);

				// If the "cd" command was given
				case 2:
					// if no directory argument was given, then change to the home directory
					// use getenv to get the path of the home directory
					if(commands[1] == 0){
						chdir(getenv("HOME"));
					} 
					// If a directory argument was given, then change to that specific directory
					else {
						chdir(commands[1]);
					}
					break;

				// If the "status" command is given
				case 3:
					// save and display the return status of the most recent command per
					// assignment requirements
					childStatus = checkStatus(childExitMethod);
			}
		}
	}

	return 0;
}



/*
 * 	Function Name: getUserInput()
 *	Description: This function will receive user input that represents the bash commands the user wants
 *		execute. 	
 *	Preconditions: The only precondition is that a char array is initialized and passed into the function
 * 		to be filled by the function.
 *	Postconditions: The char array will hold the exact input provided by the user on the command line. 
 *	Returns: none
*/
void getUserInput(char userInput[]){

	// required format for the output and receive user input
	printf(": ");
	fflush(stdout);
	fgets(userInput, MAX_LENGTH, stdin);

	// remove the trailing newline so the string is easier to handle
	// credit: https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
	userInput[strcspn(userInput, "\n")] = 0;
}



/*
 *	Function Name: parseInput()
 *	Description: This function will take the user provided input and break the individual words and
 *		symbols into individual strings. 
 *	Preconditions: An array holding the user's input must be filled and passed in. An array of pointers
 *		must also be initialized and passed in to be filled by the function.
 *	Postconditions: The array of pointers will be filled with individual commands and symbols to be used
 *		elsewhere in the program.
 *	Returns: an integer representing the total number of commands/symbols in the array of pointers
 *	Credit: https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
*/
int parseInput(char userInput[], char *argumentList[]){
	// tell the function what delimiter to use when splitting up the char array
	char *ptr = strtok(userInput, " ");

	int totalArgs = 0;

	// iterate through the user input and split the char array into individual pointers based on spaces
	// found. Increment the string counter after each pointer is copied. 
	while(ptr != NULL){
		argumentList[totalArgs] = ptr;
		ptr = strtok(NULL, " ");
		totalArgs++;
	}

	return totalArgs;
}



/*
 *	Function Name: checkForBuiltIn()
 *	Description: This function will check whether or not the user provided any built in commands in
 *		their input.
 *	Preconditions: An array of pointers containing a list of individual commands/symbols must be filled
 *		and passed into the function to check for the built-in commands.
 *	Postconditions: No changes to variables will happen, but the function will determine the existence of
 *		any built-ins.
 *	Returns: an integer representing the index of the matching built-in command found in the list, or -1 
 *		if none are found.
*/
int checkForBuiltIn(char *argumentList[]){
	// set this to -1, assuming there's no match since they haven't been check yet
	int command = -1;

	// Compare the first string in the array of pointers to each of the three supported built-in commands
	// If a match is found, assign the respective index to the "command" variable to let the calling
	// function know which command was found.
	if(strcmp(argumentList[0], "exit") == 0){
		command = 1;
	} else if(strcmp(argumentList[0], "cd") == 0){
		command = 2;
	} else if(strcmp(argumentList[0], "status") == 0){
		command = 3;
	} else {
		command = -1;
	}

	return command;
}



/*
 *	Function Name: checkInputRedirect()
 *	Description: This function will check whether the user has entered any commands to redirect the
 *		standard input.
 *	Preconditions: An array of pointers containing a list of individual commands/symbols must be filled
 *		and passed in along with the total number of arguments provided.
 *	Postconditions: Nothing will be changed, but the function will determine in which position (if any)
 *		exists a redirection of standard input.
 *	Returns: an integer representing the position of an existing input redirection, or -1 if one doesn't
 *		exist.
*/
int checkInputRedirect(char *argumentList[], int totalArguments){
	int i = 0;

	// Assume redirect doesn't exists since it hasn't been checked yet
	// If no redirect is found then -1 will be returned to signal that
	int position = -1;

	// Check each string in the array if it matches the symbol "<" to represent a redirect
	// Save the position if a match is found.
	for(i = 0; i < totalArguments; i++){
		if(strcmp(argumentList[i], "<") == 0){
			position = i;
		}
	}

	return position;
}



/*
 *	Function Name: checkOutputRedirect()
 *	Description: This function will check whether the user has entered any commands to redirect the
 *		standard output.
 *	Preconditions: An array of pointers containing a list of individual commands/symbols must be filled
 *		and passed in along with the total number of arguments provided.
 *	Postconditions: Nothing will be changed, but the function will determine in which position (if any)
 *		exists a redirection of standard output.
 *	Returns: an integer representing the position of an existing output redirection, or -1 if one doesn't
 *		exist.
*/
int checkOutputRedirect(char *argumentList[], int totalArguments){
	int i = 0;

	// Assume redirect doesn't exists since it hasn't been checked yet
	// If no redirect is found then -1 will be returned to signal that
	int position = -1;


	// Check each string in the array if it matches the symbol ">" to represent a redirect
	// Save the position if a match is found.
	for(i = 0; i < totalArguments; i++){
		if(strcmp(argumentList[i], ">") == 0){
			position = i;
		}
	}

	return position;
}



/*
 *	Function Name: updateArgList()
 *	Description: This function will remove any redirection symbols, input/output files associated with
 *		redirection, and ampersands.
 *	Preconditions: An empty array of pointers, filled array of commands, signals for the presence of
 *		redirection symbols, and the total number of arguments must be passed in for use within the function
 *	Postconditions: Only the empty array of pointers will be filled with the users commands excluding redirection
 *		symbols, redirected files, and ampersands. 
 *	Returns: an integer representing the number of arguments in the newly created array of argument pointers
*/
int updateArgList(char *argumentList[], char *newArgList[], int inRedirect, int outRedirect, int totArgs){
	int i = 0;
	int updatedIndex = 0;


	// If both input and output redirection symbols are found, iterate through the raw argument
	// list and copy every argument besides the redirection symbols and redirection files
	// This ensures that we only send the main command into execvp() since we'll be redirecting
	// input and output elsewhere. 
	if(inRedirect != -1 && outRedirect != -1){
		for(i = 0; i < totArgs; i++){
			if(strcmp(argumentList[i], "<") != 0 && strcmp(argumentList[i], ">") != 0 && strcmp(argumentList[i], argumentList[inRedirect+1]) != 0 && strcmp(argumentList[i], argumentList[outRedirect+1]) != 0){
				newArgList[updatedIndex] = argumentList[i];

				// increment the number of arguments (same for each section)
				updatedIndex++;
			}
		}
	}

	// If only the input redirection symbol is present, then iterate through the raw argument list
	// and copy every argument that doesn't match the input redirection symbol
	else if(inRedirect != -1 && outRedirect == -1){
		for(i = 0; i < totArgs; i++){
			if(strcmp(argumentList[i], "<") != 0 && strcmp(argumentList[i], argumentList[inRedirect+1]) != 0){
				newArgList[updatedIndex] = argumentList[i];
				updatedIndex++;
			}
		}
	}

	// If only the output redirection symbol is present, then iterate through the raw argument list
	// and copy every argument that doesn't match the output redirection symbol
	else if(outRedirect != -1 && inRedirect == -1){
		for(i = 0; i < totArgs; i++){
			if(strcmp(argumentList[i], ">") != 0 && strcmp(argumentList[i], argumentList[outRedirect+1]) != 0){
				newArgList[updatedIndex] = argumentList[i];
				updatedIndex++;
			}
		}
	}

	// If neither redirection symbol exists, then iterate through the list and copy every argument that
	// isn't an ampersand (signalling to the run the command in the background)
	else{
		for(i = 0; i < totArgs; i++){
			if(strcmp(argumentList[i], "&") != 0){
				newArgList[updatedIndex] = argumentList[i];
				updatedIndex++;
			}
		}
	}

	return updatedIndex;
}



/*
 *	Function Name: checkStatus()
 *	Description: This will check the exit/termination status of the most recently 
 *		terminated child process. 
 *	Preconditions: An integer representing the exit status will be passed into the function
 *	Postconditions: The exit status will be deciphered
 *	Returns: an integer representing the deciphered status will be returned to be used in
 *		the calling function
*/
int checkStatus(int exitMethod){
	int status = 0;

	// Check and save exit status if the process exited normally 
	// output the appropriate exit value statement
	if(WIFEXITED(exitMethod) != 0){
		status = WEXITSTATUS(exitMethod);
		printf("exit value %d\n", status);
	}

	// Check and save exit status if the process was terminated by some signal
	// output the appropriate statement
	else if(WIFSIGNALED(exitMethod) != 0){
		status = WTERMSIG(exitMethod);
		printf("terminated by signal %d\n", status);
		fflush(stdout);
	}

	return status;
}



/*
 *	Function Name: replace()
 *	Description: This function will replace any instance of "$$" with the process
 *		ID of the shell itself.
 *	Preconditions: A final array of argument pointers, the position of the string holding
 *		the "$$" substring, and the shell's process ID must be determined and passed in to
 *		be used in the function.
 *	Postconditions: The string containing the substring "$$" will be updated and have the 
 *		shell's process ID appended to it and the "$$" deleted.
 *	Returns: none
 *	Credit: https://stackoverflow.com/questions/14544920/how-do-i-remove-last-few-characters-from-a-string-in-c
*/
void replace(char *args[], int argPosition, int parentPID){
	// initialize and clear buffer to hold string version of pid
	char pidString[15];
	memset(pidString, 0, 15);

	// turn pid integer into a string so it can be appended in place of
	// "$$" instance
	sprintf(pidString, "%d", parentPID);

	// remove "$$" and concatenate the pid string to the original string
	// This implements the required expansion of the "$$"
	int length = strlen(args[argPosition]);
	args[argPosition][length-2] = '\0';
	strcat(args[argPosition], pidString);
}



/*
 *	Function Name: checkBackground()
 *	Description: This function will check for the existence of the "&" symbol, signaling
 *		that the command should be ran i nthe background.
 *	Preconditions: An array of argument pointers and number of total arguments need to be
 *		passed in for use within the function. 
 *	Postconditions: Nothing will be changed, but the function will determine at which position
 *		, if any, exists a "&".
 *	Returns: an integer representing the position of the "&", or -1 if none exists
*/
int checkBackground(char *argumentList[], int totalArguments){
	int i = 0;

	// Assume ampersand doesn't exists since it hasn't been checked yet
	// If no ampersand is found then -1 will be returned to signal that
	int position = -1;

	// Check each string in the array if it matches the symbol "&"
	// Save the position if a match is found.
	for(i = 0; i < totalArguments; i++){
		if(strcmp(argumentList[i], "&") == 0 && strcmp(argumentList[0], "echo") != 0){
			position = i;
		}
	}

	return position;
}



/*
 *	Function Name: checkTerminatedProcesses
 *	Description: This function will periodically check if any background processes have
 *		terminated.
 *	Preconditions: An integer for the exit status must be created and passed in to be changed.
 *	Postconditions: The exit status will be updated and changed depending if any process has
 *		actually terminated.
 *	Returns: none. 
*/
void checkTerminatedProcesses(int exitMethod){

	usleep(250000);
	// check if any process has terminated
	int exitPID = waitpid(-1, &exitMethod, WNOHANG);

	// if a positive pid is returned then we know a process terminated
	while(exitPID > 0){

		// get the exit status of whatever process exited and display it
		// this is a required functionality
		printf("background pid %d is done: ", exitPID);
		int status = checkStatus(exitMethod);

		// Check for any other processes that may have terminated and go through the
		// same process if one is found again. 
		exitPID = waitpid(-1, &exitMethod, WNOHANG);
	}
}



/*
 *	Function Name: catchSIGINT()
 *	Description: This function will catch and handle a SIGINT keyboard signal
 *	Precondition: No other preconditions other than a SIGINT signal being sent
 *		from keyboard input.
 *	Postcondition: A message confirming the signal was caught will be displayed
 *	Returns: none
*/
void catchSIGINT(int signo){
	// display the message when SIGINT is received
	// this tells the user that a child process was terminated by SIGINT
	char *message = "terminated by signal 2\n";
	write(STDOUT_FILENO, message, 23);
}



/*
 * 	Function Name: catchStop()
 *	Description: This function will catch and handle a SIGTSTP keyboard signal.
 *		It will toggle foreground only mode on/off depending on its current state
 *	Preconditions: The only precondition is that a SIGTSTP signal be issue by the
 *		user from the keyboard
 *	Postconditions: The foreground variable will be turned on or off depening on its
 *		state when the next SIGTSTP signal is received
 *	Returns: none 
*/
void catchStop(int signo){

	// this will only run if foreground-only mode is currently off
	// this will satisfy the requirement that the SIGTSTP should turn foreground
	// only mode on if its received
	if(foregroundOnlyMode == 0){

		// switch fg only mode on
		foregroundOnlyMode = 1;

		//display required message stating fg only mode is now on
		// this is required if user enters ctrl-z
		char *message = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 49);
	}

	// this runs if foreground only mode is already on. It will turn it off, again
	// honoring the use of the "&" symbol that states that the process should be run
	// in the background
	else if(foregroundOnlyMode == 1){

		// switch fg only mode off
		foregroundOnlyMode = 0;

		//display required message stating fg only mode is now off
		// this is required if user enters ctrl-z
		char *message = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 29);
	}
}
