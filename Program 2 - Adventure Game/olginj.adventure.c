/*
 *	Name: John Olgin
 *	Program: Assignment 2 - olginj.adventure.c
 *	Date: 7/17/19
 *	Email: olginj@oregonstate.edu
 *	Description: This will simulate an adventure game via text. The user will be able to traverse through
 *		a group of rooms until they have found the end room, where the user will have won the game. Once
 *		the user finds this final room, they will be shown their path along with how many steps they took
 *		to get there. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <ctype.h>

struct room{
	char *roomName;
	char *roomType;
	int connections[6];
	int numberOfConnections;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char *getFileName();
void setRoomData(char *roomFile, struct room roomsInGame[], char *roomNames[], char *types[]);
int checkForRoomName(char fileString[], char *roomNames[]);
int checkForType(char fileString[], char *roomTypes[]);
void playGame(struct room roomsInGame[], char *roomNames[]);
int checkConnection(int currentPosition, struct room roomsInGame[], char *allRooms[], char userChoice[]);
void *writeTime();
void displayTime();
void printCurrentRoomInfo(struct room rooms[], char *roomNames[], int currentRoom);


int main(){
	// Initialize the arrays to be used and passed into the primary program functions
	char *listOfRooms[10] = {"Horror", "Comedy", "Action", "Western", "Thriller", "Romance", "Doc", "Drama", "Crime", "SciFi"};
	char *roomTypes[3] = {"START_ROOM", "MID_ROOM", "END_ROOM"};
	struct room roomsInGame[7];

	// get most recently modified room directory
	char *roomFile = getFileName();

	// Fill the array of room structs with all appropriate room information
	setRoomData(roomFile, roomsInGame, listOfRooms, roomTypes);

	// run the actual game
	playGame(roomsInGame, listOfRooms);

	// destroy the mutex
	pthread_mutex_destroy(&mutex);

	return 0;
}



/*
 *	Function Name: getFileName()
 *	Description: This function will find the directory most recently modified by the olginj.buildrooms.c program.
 *		This will allow another function to read directly from the room files in the directory to create the local
 *		room data.
 *	Preconditions: This function requires the olginj.buildrooms.c program to be compiled and ran before its called.
 *		This is because it needs at least one directory with "olginj.rooms." at the beginning of the directory name
 *		so the function can return its name as a string to the main function.
 *	Postconditions: No files will be changed, but the only variable that will be changed is the fileBuffer string. 
 *		Other than that, nothing will be modified.
 *	Returns: This function returns a string representing the most recently modified directory to be used in another 
 *		function.
 *	Credit: This code was heavily influenced by this link - https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
*/

char *getFileName(){
	// Create the pointer to the directory information and open current directory to read names
	struct dirent *de;
	DIR *dr = opendir(".");


	// prepare buffer that holds the file name and allocate space
	char *fileBuffer = NULL;
	fileBuffer = malloc(sizeof(char)*(50));


	// Check that the pointer isn't null and that it's pointing to the necessary directory
	if(dr == NULL){
		printf("Could not open current directory\n");
	}


	long int mostRecent = 0;

	// Go through every file that is preceded by "olgin.rooms."
	// So we're only comparing room directories
	while((de = readdir(dr)) != NULL){
		if(strncmp(de->d_name, "olginj.rooms", 12) == 0){

			// get the statistics for each file
			struct stat st;
			stat(de->d_name, &st);


			// Compare the st_mtime to the most recent time so far
			// Update most recent time and copy file name if a more recent file is found
			if(st.st_mtime > mostRecent){
				sprintf(fileBuffer, "%s", de->d_name);
				mostRecent = st.st_mtime;
			}
		}
	}

	closedir(dr);

	return fileBuffer;
}




/*
 *	Function Name: setRoomData()
 *	Description: This function will read from the seven files created by the olginj.buildrooms.c program.
 *		It will use the file contents to build the set of rooms to be used in the adventure game itself.
 *	Preconditions: For this function to run correctly, the getFileName() function first needs to provide the 
 *		name of the most recently modified directory via a string. This is the directory this function will
 *		read from.
 *	Postconditions: The local array of room structs will be filled with the data from the seven files created
 *		by the buildrooms program. 
 *	Returns: Nothing
 *	Credit: The code that reads the lines from the file was heavily influenced by this link
 *		https://www.daniweb.com/programming/software-development/code/216411/reading-a-file-line-by-line
*/
void setRoomData(char *directoryName, struct room roomsInGame[], char *roomNames[], char *types[]){
	// Set the file name of the most recently modified file, determined by getFileName()
	// copy the directory name into the buffer and set it as working directory
	char *fileDirectory = directoryName;
	char directoryBuffer[100];
	sprintf(directoryBuffer, "%s", fileDirectory);
	chdir(directoryBuffer);

	// Set a pointer to the current directory and directory information
	DIR *dr = opendir(".");
	struct dirent *de;

	// Check the directory is properly pointed to
	if(dr == NULL){
		printf("Error opening directory\n");
	}

	// Create string that will hold the current line read from the room files
	char stringBuffer[100];

	int roomIndex = 0;


	// Iterate through all regular files (not directories) in current directory
	while((de = readdir(dr)) != NULL){
		if (de->d_type == DT_REG){

			// Track connections added for each room
			int connectionIndex = 0;

			// Open the current file to be read from
			FILE *fptr = fopen(de->d_name, "r");

			// While lines are being read from the current room file
			// This ensures we read through the entire file
			while(fgets(stringBuffer, sizeof stringBuffer, fptr) != NULL){

				// If ROOM NAME is in the line, we know this is the line that will hold 
				// the rooms name. Retrieve the correct name and set it as the current room's
				// name. 
				if(strncmp(stringBuffer, "ROOM NAME", 9) == 0){
					int namePosition = checkForRoomName(stringBuffer, roomNames);
					roomsInGame[roomIndex].roomName = roomNames[namePosition];
				}

				// If CONNECTION is in the line, we know this is the line that holds the name of 
				// a room that's connected to the current room we're filling info for. Retrieve the
				// correct name and fill it in as a connection for the current room and increment
				// the room's number of connections.
				else if(strncmp(stringBuffer, "CONNECTION", 10) == 0){
					int namePosition = checkForRoomName(stringBuffer, roomNames);
					roomsInGame[roomIndex].connections[connectionIndex] = namePosition;

					connectionIndex++;
				}

				// If ROOM TYPE is in the line, we know this line holds the type of room the
				// current room is. Therefore, we check the line for that type. Retrieve the 
				// correct type and set it as the current room's type. 
				else if(strncmp(stringBuffer, "ROOM TYPE", 9) == 0){
					int typePosition = checkForType(stringBuffer, types);
					roomsInGame[roomIndex].roomType = types[typePosition];

					// Since we know this is last line in the room file, we can set the number
					// of connections for the current room and iterate to the next room.
					roomsInGame[roomIndex].numberOfConnections = connectionIndex;
					roomIndex++;
				}
			}

			fclose(fptr);
		}
	}	
}



/*
 *	Function Name: CheckForRoomName()
 *	Description: This function will check for a matching name in the list of names. It will compare
 *		the name given in the room file to every possible name and tell the controlling program which
 *		name it matches.
 *	Preconditions: A file needs to accurately be read from the room file and determined to hold the 
 *		name of a room. It must then be passed into this function along with the a list of all names.
 *	Postconditions: The calling function will be provided the position of the name provided in the
 		file. This will allow the program to fill the room names with the right names.
 *	Returns: This will return the exact position (an integer) in the room names array that matches.
*/
int checkForRoomName(char fileString[], char *roomNames[]){
	int nameListPosition = 0;
	int i = 0;

	// Iterate through all possible room names in the array of strings representing names
	// Check if any of the room names exist in the string, if so, save the matching position to be
	// returned to the calling function
	for(i = 0; i < 10; i++){
		char *exists = strstr(fileString, roomNames[i]);

		if(exists != NULL){
			nameListPosition = i;
		} 
	}

	return nameListPosition;
}



/*
 *	Function Name: CheckForType()
 *	Description: This function checks for a matching type according to the room type provided in the 
 *		room file.
 *	Preconditions: A file needs to accurately be read from the room file and determined to hold the 
 *		type of a room. It must then be passed into this function along with the a list of all types.
 *	Postconditions: The calling function will be provided the position of the type provided in the
 		file. This will allow the program to fill the room types with the right types.
 *	Returns: This will return the exact position (an integer) in the room types array that matches.
*/
int checkForType(char fileString[], char *roomTypes[]){
	int typePosition = 0;
	int i = 0;

	// Iterate through all possible room types in the array of strings representing types. Check if
	// current room type exists in the read in string. If so, this means this is the room type for
	// the current room. So return the matching position from the room types array to calling function
	for(i = 0; i < 3; i++){
		char *exists = strstr(fileString, roomTypes[i]);

		if (exists != NULL){
			typePosition = i;
		}
	}

	return typePosition;
}



/*
 *	Function Name: checkConnection()
 *	Description: This function will match each of the rooms' connections to rooms that are actually used
 *		in the game. 
 *	Preconditions: A file needs to accurately be read from the room file and determined to hold all of the 
 *		rooms connections. It must then be passed into this function along with the position the connections
 *		should be added to.
 *	Postconditions: The array of room structures will be accurately filled with the connections for each room.
 *	Returns: An integer representing where the name of the connection exists in the overall list of room
 *		names.
*/
int checkConnection(int currentPos, struct room rooms[], char *allRooms[], char userChoice[]){
	int connectPosition = -1;
	int i = 0;

	// Iterate through the room's connections
	for(i = 0; i <  rooms[currentPos].numberOfConnections; i++){

		// If the user's choice matches a name from the full list of names, we want to
		// check that it exists within the names chosen for this game. This will only check
		// the rooms that are in the current room's connection array
		if(strcmp(userChoice, allRooms[rooms[currentPos].connections[i]]) == 0){
			int j = 0;

			// Iterate through all the rooms in the array of room structs for this game. 
			// Save the position of the room whose name matches the string if
			// one exists, so it can be returned to the calling function.
			for(j = 0; j < 7; j++){
				if(strcmp(userChoice, rooms[j].roomName) == 0){
					connectPosition = j;
				}
			}
		}
	}

	return connectPosition;
}




/*
 *	Function Name: writeTime()
 *	Description: This is a dedicated thread function. It will write the current date and time to a 
 *		a file named currentTime.txt
 *	Preconditions: Both a thread and mutex need to be initialized before this function can be called.
 *	Postconditions: A new text file named currentTime.txt will be created and a line of text with
 *		the date and time will be written on the first line.
 *	Returns: nothing
 *	Credit: The code for retrieving the time was influenced by the following link
 *		https://stackoverflow.com/questions/5141960/get-the-current-time-in-c
*/
void *writeTime(){

	// Initialize a time_t object, save the current raw time to it, then use the tm struct
	// to properly format it per the assignment in structions 
	time_t nowTime;
	time(&nowTime);
	struct tm *newTime = localtime(&nowTime);


	// Create the buffer to hold the string that will be written
	// Save the time to the string with the proper format
	char timeBuffer[60];
	memset(&timeBuffer, '\0', sizeof(timeBuffer));
	strftime(timeBuffer, 55, "%l:%M%p, %A, %B %d, %Y\n", newTime);

	// Make AM/PM lowercase
	timeBuffer[5] = tolower(timeBuffer[5]);
	timeBuffer[6] = tolower(timeBuffer[6]);

	// Lock the mutex so no other process can access the data within the critical section
	pthread_mutex_lock(&mutex);

	// Create and open the file for writing to write the string saved in the string buffer
	FILE *fptr = fopen("currentTime.txt", "w");
	fwrite(timeBuffer, 1, sizeof(timeBuffer), fptr);
	fclose(fptr);

	// Unlock the mutex so other processes can now access the newly created file
	pthread_mutex_unlock(&mutex);

	pthread_exit(NULL);
}




/*
 *	Function Name: displayTime()
 *	Description: This function will read from the currentTime.txt file and display the time and
 *		date on the screen for the user to see.
 *	Preconditions: A mutex needs to be initialized and the currentTime.txt file needs to be created
 *		and written to prior to calling this function.
 *	Postconditions: The current date and time will be written to the screen for the user to see.
 *		The text file won't be modified at all, just read from. 
 *	Returns: nothing
*/
void displayTime(){
	char timeBuffer[50];

	// Lock the mutex so no other processes can access the data in this critical section
	pthread_mutex_lock(&mutex);

	// Open the time file for reading and get the very first line and print it to screen
	FILE *fptr = fopen("currentTime.txt", "r");
	fgets(timeBuffer, 50, fptr);
	printf("\n%s\n", timeBuffer);

	fclose(fptr);

	// unlock the mutex so the data (time file) is accessible by other processes.
	pthread_mutex_unlock(&mutex);
}



/*
 *	Function Name: printCurrentRoomInfo()
 *	Description: This function will print the room name and all the room's connections of the
 *		current position in the array of room structs. 
 *	Preconditions: All room info needs to be included for each room in the game before this function
 *		is called.
 *	Postconditions: None of the arrays will be affected or changed, but the room's information will
 *		be displayed on the screen for the user to see.
 *	Returns: nothing
*/
void printCurrentRoomInfo(struct room roomsInGame[], char *roomNames[], int currentRoom){
	// Display required text on screen
	printf("\n");
	printf("CURRENT LOCATION: %s\n", roomsInGame[currentRoom].roomName);
	printf("POSSIBLE CONNECTIONS: ");


	// Iterate through all the connections to the current room and print their
	// room names to the screen
	int j = 0;
	for(j = 0; j < roomsInGame[currentRoom].numberOfConnections; j++){
		printf("%s", roomNames[roomsInGame[currentRoom].connections[j]]);
		int check = roomsInGame[currentRoom].numberOfConnections - 1;

		// To maintain the correct format, place a comma and space after every name except last,
		// and only print a period after the very last connection name.
		if(j != check){
			printf(", ");
		} else {
			printf(".");
		}
	}
}


/*
 *	Function Name: playGame()
 *	Description: This function will control the actual flow of the game. It will receive user input,
 *		display the appropriate prompts, and call the necessary functions to keep the game going.
 *	Preconditions: Before this function can be called, all rooms in the array of room structs must be
 *		filled with the correct room information. If this is achieved, the function will run the game
 *		correctly.
 *	Postconditions: The user will be able to navigate through the rooms until they reach the end.
 *		They will also be able to see their path and how many steps they took. 
 *	Returns: nothing
*/
void playGame(struct room roomsInGame[], char *roomNames[]){

	// Initialize the array to hold the names of the rooms in each step
	char *stepRooms[100];

	// This is the buffer that will hold the user's input when they choose
	// which room they want to go to next
	char nextRoom[30];

	// initialize a thread to use when the use enters "time"
	pthread_t timeThread;

	// An earlier function changed the current directory
	// This changes the working directory back to the original directory
	chdir("..");



	int startRoomFound = 0;
	int i = 0;

	// This will iterate through the list of rooms until it finds the room
	// with the room type "START_ROOM". It will set this as the user's starting place
	while(startRoomFound != 1){
		if(strcmp(roomsInGame[i].roomType, "START_ROOM") == 0){
			startRoomFound = 1;
			break;
		}

		i++;
	}

	
	// This holds the number of steps taken by the user 
	int stepsTaken = 0;


	// This is the main control loop for the game itself. It will run until the user 
	// correctly inputs the room name of the room with room type = "END_ROOM"
	while(strcmp(roomsInGame[i].roomType, "END_ROOM") != 0){

		printCurrentRoomInfo(roomsInGame, roomNames, i);

		// Prompt the user for their choice for the next room and accept their input
		// This input will be evaluated for a matching connection or the word "time"
		printf("\nWHERE TO? >");
		scanf("%s", nextRoom);



		// As long as the user enters "time", this section of code will run
		while(strcmp(nextRoom, "time") == 0){

			// This will start the thread that runs the function writeTime()
			// This is where the program will write the current time in the correct
			// format to currentTime.txt per assignment instructions
			pthread_create(&timeThread, NULL, writeTime, NULL);

			// Allow the 2nd thread to gain the lock to print time
			usleep(600000);

			// Prints out the time and date on screen in the required format
			displayTime();

			// This will force the main program to wait here until the time thread resolves
			pthread_join(timeThread, NULL);

			printf("Where to? >");
			scanf("%s", nextRoom);
		}

		// This will return the position of the name for the connection in the array of 
		// room structs. It will return -1 if an invalid name is given.
		int nextPosition = checkConnection(i, roomsInGame, roomNames, nextRoom);


		// This forces the user to enter a name until it matches the name to a valid
		// connection for the current room.
		while(nextPosition == -1){
			printf("\n");
			printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
			printCurrentRoomInfo(roomsInGame, roomNames, i);

			printf("\nWHERE TO? >");
			scanf("%s", nextRoom);

			// This is the exact same while loop for "time" as above
			while(strcmp(nextRoom, "time") == 0){
				pthread_create(&timeThread, NULL, writeTime, NULL);
				usleep(600000);
				displayTime();

				pthread_join(timeThread, NULL);

				printf("Where to? >");
				scanf("%s", nextRoom);
			}

			nextPosition = checkConnection(i, roomsInGame, roomNames, nextRoom);
		}

		// set the index equal to the valid next position provided by the user
		// this is so we can properly print the next room's info and analyze its
		// connections in the next iteration.
		i = nextPosition;


		// This saves the name of the current room to the array of steps take
		// It also increments the number of steps that have been taken
		// These are necessary since they need to be displayed at the end of game
		stepRooms[stepsTaken] = roomsInGame[i].roomName;
		stepsTaken++;
	}


	// Tell user they have won, show rooms stepped through and how many steps taken
	printf("\n");
	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", stepsTaken);

	int j = 0;
	for(j = 0; j < stepsTaken; j++){
		printf("%s\n", stepRooms[j]);
	}
}




