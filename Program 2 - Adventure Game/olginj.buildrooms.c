/*	-- Author: John Olgin
	-- Description: This program is a rendition of a text-based adventure game. The user's primary goal
	is to start the game and navigate through the rooms to find the ending room. Achieving this means 
	the user has won the game.
	-- Date: 7/15/19
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>


struct room{
	char *roomName;
	char *roomType;
	int connections[6];
	int numberOfConnections;
};

void generateRooms(struct room roomsInGame[], char *listOfRooms[]);
void assignRoomType(struct room roomsInGame[], char *roomType[]);
void createConnections(struct room roomsInGame[]);
void writeToFile(struct room roomsInGame[], char *roomList[], char *fileName[]);


int main(){
	// Initialize the arrays that hold room names, file names, and room types strings to be passed
	// into the functions
	char *listOfRooms[10] = {"Horror", "Comedy", "Action", "Western", "Thriller", "Romance", "Doc", "Drama", "Crime", "SciFi"};
	char *fileNames[10] = {"Horror_room", "Comedy_room", "Action_room", "Western_room", "Thriller_room", "Romance_room", "Doc_room", "Drama_room", "Crime_room", "SciFi_room"};
	char *roomTypes[3] = {"START_ROOM", "MID_ROOM", "END_ROOM"};

	// seed time for rand function
	srand((unsigned) time(0));

	// initialize array that will hold the array of room structs
	struct room roomsInGame[7];

	generateRooms(roomsInGame, listOfRooms);

	assignRoomType(roomsInGame, roomTypes);

	createConnections(roomsInGame);

	writeToFile(roomsInGame, listOfRooms, fileNames);

	return 0;
}

/*
 *	Function Name: generateRooms()
 *	Description: This function will randomly generate 7 rooms of the full set of 10. It will
 *		ensure no room is used twice and that only 7 are used in total. 
 *	Preconditions: The only requirement for this function is that a list of room names are 
 *		hardcoded for the function to choose from.
 *	Postconditions: The array of room structures will be filled with room names that are only
 *		used once for the seven rooms.
 *	Returns: nothing
*/
void generateRooms(struct room roomsInGame[], char *listOfRooms[]){

	int i = 0;

	// Iterate through all rooms in the array of room structs.
	for (i = 0; i < 7; i++){

		// Generate a random number representing the room name for the current array position
		int randNum = (rand() % (9 - 1 + 1)) + 1;
		int alreadyUsed = 1;

		// Initialized the number of connections for each room to 0
		// This is just for good practice
		roomsInGame[i].numberOfConnections = 0;

		// Run this code only if at least one room has already been generated
		if (i > 0) {

			// If the room name has already been used, generate a random room name until
			// one is found that has not yet been used. 
			while(alreadyUsed == 1) {
				randNum = (rand() % (9 - 1 + 1)) + 1;

				int j;
				int roomUseCheck = 0;

				// Iterate through all current room names checking if the randomly generated
				// room has already been used. Increment roomUseCheck if it has been used to
				// force the while loop to pick another room name. 
				for (j = 0; j < i; j++){
					if (listOfRooms[randNum] == roomsInGame[j].roomName){
						roomUseCheck++;
						j = i;
					}
				}

				// If no matching room was found, let the while loop know and set the current
				// room's name to the randomly generated room name.
				if (roomUseCheck == 0){
					alreadyUsed = 0;
					roomsInGame[i].roomName = listOfRooms[randNum];
				}
			}
		}
		else {
			// This section only runs on the very first iteration. The first room can automatically
			// be assigned the first randomly generated name since no other rooms have names yet.
			roomsInGame[i].roomName = listOfRooms[randNum];
		}
	}
}


/*
 *	Function Name: assignRoomType()
 *	Description: This function will randomly assign room types to each room. It will provide 
 *		exactly 1 end room and 1 start room. Therefore, it will assign 5 mid rooms.
 *	Preconditions: The only precondition is that a hardcoded list of all possible room types
 *		is provided to the function.
 *	Postconditions: Each room in the array of room structs will have a valid room type, and
 *		only 1 room will be the start and 1 will be the end. 
 *	Returns: nothing
*/
void assignRoomType(struct room roomsInGame[], char *roomTypes[]){
	// Generate two random positions, one for the start room and one for end room
	// This ensures that random rooms are start and end every time
	int randStartRoom = (rand() % (6 - 0 + 1)) + 0;
	int randEndRoom = (rand() % (6 - 0 + 1)) + 0;

	// Check if the random positions are the same (the same room can't be start and end)
	if (randEndRoom == randStartRoom) {
		int sameRoom = 1;

		// Generate a new end room position until its not the same as the start room
		while(sameRoom == 1){
			randEndRoom = (rand() % (6 - 0 + 1)) + 0;

			if(randEndRoom != randStartRoom){
				sameRoom = 0;
			}
		}
	}

	// Assign the start and end room types to the two randomly generated positions	
	roomsInGame[randStartRoom].roomType = roomTypes[0];
	roomsInGame[randEndRoom].roomType = roomTypes[2];


	// Assign the remaining rooms as mid rooms
	// This will ensure there are always 5 mid rooms and only 1 start and 1 end room
	int i = 0;
	for (i = 0; i < 7; i++){
		if(i != randStartRoom && i != randEndRoom){
			roomsInGame[i].roomType = roomTypes[1];
		}
	}
}




/*
 *	Function Name: createConnections()
 *	Description: This function will randomly generate and assign connections for each room.
 *		It will provide between 3 and 6 connections for each room, with each connection 
 *		being tied to both rooms in the connection. Therefore, the connections will be included
 *		in both rooms' connection arrays.
 *	Preconditions: The rooms in the array of room structs need to be assigned names before this
 *		function is called.
 *	Postconditions: All the rooms' connection arrays will be filled with valid connections to other
 *		rooms. This will be achieved for each room in the array of rooms. 
 *	Returns: Nothing
*/
void createConnections(struct room roomsInGame[]){
	int i = 0;


	// Loop through each room in the array of rooms
	for(i = 0; i < 7; i++){

		// Set a random number of connections for the room
		// This will only be a minimum, unless it's 6 where it'll also be max
		int randNumConnections = (rand() % (6 - 3 + 1)) + 3;


		// Loop until the current room has at least the random number of connections generated
		// Making this value a minimum as opposed to an exact value takes a constraint off the program. 
		while(roomsInGame[i].numberOfConnections < randNumConnections){
			int newConnect = (rand() % (6 - 0 + 1) + 0);


			// Check the random new connection doesn't match the current room and that it
			// doesnt't have 6 connections already (make sure it isn't full). We don't want
			// rooms connecting to themselves.
			if(newConnect != i && roomsInGame[newConnect].numberOfConnections != 6){
				int j = 0;
				int connectExists = 0;

				// Check that the new connection doesn't already exist with the current room
				// Don't want to duplicate connections.
				for(j = 0; j < roomsInGame[i].numberOfConnections; j++){
					if(newConnect == roomsInGame[i].connections[j]){
						connectExists++;
					}
				}

				// If there's no current connection with new random room, create a connection
				// and add the connection to boths rooms' connections arrays. 
				if(connectExists == 0){
					roomsInGame[i].connections[roomsInGame[i].numberOfConnections] = newConnect;
					roomsInGame[newConnect].connections[roomsInGame[newConnect].numberOfConnections] = i;

					roomsInGame[i].numberOfConnections++;
					roomsInGame[newConnect].numberOfConnections++;
				}
			}
		}
	}
}



/*
 *	Function Name: writeToFile()
 *	Description: This function will write the rooms' information to 7 different files, with file names
 *	respective to each room's name. These files will be placed into a newly created directory 
 *		to be read from and used by the adventure program.
 *	Preconditions: All information (room names, connections and types) needs to be filled in for
 *		each room in the array of rooms for this function to run correctly.
 *	Postconditions: 7 different text files containing a rooms information will be created and placed
 *		into a newly created directory with the name "olginj.rooms.  +  processID". 
 *	Returns: nothing
 *	Credit: This was heavily influenced by these links - https://www.programiz.com/c-programming/c-file-input-output
 *		https://www.programmerfish.com/create-output-file-names-using-a-variable-in-c-c/#.XTIA-JNKh25
*/	
void writeToFile(struct room roomsInGame[], char *roomList[], char *fileName[]){
	FILE *fptr;

	// This section prepares the name for the directory to be created
	// It also appends the process ID to the directory name to differentiate it
	// Fills the directoryBuffer with the full directory name
	const char* directoryName = "olginj.rooms.";
	pid_t pid = getpid();
	char directoryBuffer[100];
	sprintf(directoryBuffer, "%s%d", directoryName, pid);


	// Creates the directory with the correct permissions
	// Makes the new directory the current working directory
	int check = mkdir(directoryBuffer, 0770);
	chdir(directoryBuffer);


	// Iterate through each room in the array of rooms
	int i = 0;
	for(i = 0; i < 7; i++){
		int j = 0;


		// Iterate through all possible room names
		for(j = 0; j < 10; j++){

			// When the room name matches a name in the list, create a file named for that room
			// This is to create the seven separate files, one for each room
			if(roomsInGame[i].roomName == roomList[j]){
				fptr = fopen(fileName[j], "w");


				// This writes the room information to each room file
				// This is written so the file formats are exactly what's required
				fprintf(fptr, "ROOM NAME: %s\n", roomsInGame[i].roomName);

				int k = 0;
				for(k = 0; k < roomsInGame[i].numberOfConnections; k++){
					fprintf(fptr, "CONNECTION %d: %s \n", k+1, roomsInGame[roomsInGame[i].connections[k]].roomName);
				}

				fprintf(fptr, "ROOM TYPE: %s\n", roomsInGame[i].roomType);

				// close the opened file
				fclose(fptr);
			}
		}
	}
}




