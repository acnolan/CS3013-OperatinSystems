/*
 * life.cpp
 * Andrew Nolan - acnolan
 * Part 2 of Homework 3
 */

#include <cstdio>
#include <fstream>
#include <iostream>
#include <semaphore.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


using namespace std;

//define the message types
#define RANGE 1
#define ALLDONE 2
#define GO 3
#define GENDONE 4
#define STOP 5

//define some values for game of life 
#define MAXGRID 40
#define EVEN 0
#define ODD 1

/*
 * struct msg
 * a struct to store message data
 */
struct msg{
	int iSender;  	//sender of the message
	int type;	//type of message
	int value1;	//first value
	int value2;	//second value
};

//The mailbox storage struct
struct msg ** mailboxes;

//the arrays of semaphores for sending and recieving
sem_t * sendSemaphore;
sem_t * recvSemaphore;

//global variable for the total number of generations
int totalGenerations;

//grids to hold the even and odd generations
int** evenGeneration;
int** oddGeneration;

//row and column global variables
int totalRows;
int totalColumns;

/*
 * SendMsg
 * sends a message to a mailbox
 * @param iTo - mailbox to send to
 * @param Msg - message to be sent
 */
void SendMsg(int iTo, struct msg *Msg){
	sem_wait(&sendSemaphore[iTo]);
	mailboxes[iTo] = Msg;
	sem_post(&recvSemaphore[iTo]);
}

/*
 * RecvMsg
 * recieves a message
 * @param iFrom - mailbox to recieve message from
 * @param Msg   - message structure to fill in with recieved message
 */
void RecvMsg(int iFrom, struct msg *Msg){
	sem_wait(&recvSemaphore[iFrom]);
	*Msg = *mailboxes[iFrom];
	delete mailboxes[iFrom];
	sem_post(&sendSemaphore[iFrom]);
}

/*
 * printGrid
 * prints the grid for the current generation
 * @param generation - whether we should be on the odd or even grid
 */
void printGrid(int generation){
	for(int i = 0; i<totalRows; i++){
		for(int j=0;j<totalColumns;j++){
			if(generation == ODD){
				cout << oddGeneration[i][j] << " ";
			}else{
				cout << evenGeneration[i][j] << " ";
			}
		}
		cout << "\n";
		cout.flush();
	}
}

/*
 * countNeighbors
 * counts the number of neighbors a given cell has
 * @param row - the row of the cell
 * @param column - the column of the cell
 * @param generation - if it is an even or odd generation
 * @return the number of neighbors the cell has
 */
int countNeighbors(int row, int column, int generation){
	int neighbors = 0; //the counter for number of neighbors
	for(int i=row-1;i<row+2; i++){
		for(int j=column-1;j<column+2;j++){
			//make sure we aren't out of bounds
			if(i>-1 && j>-1 && i<totalRows && j<totalColumns){
				//don't double count yourself
				if(!(i==row && column==j)){
					if(generation == EVEN){
						neighbors+=evenGeneration[i][j];
					}else{
						neighbors+=oddGeneration[i][j];
					}
				}
			}
		}
	}
	return neighbors;
}

/*
 * childTask
 * This is the task that the child threads complete before sending a message back to the parent
 * for game of life it is processing it's specified rows
 * @param Id - the mailbox id of the child thread
 */
void* childTask(void* Id){
	int threadId = (int)Id;
	struct msg recievedMessage;
	RecvMsg(threadId, &recievedMessage);
	int firstRow=recievedMessage.value1;
	int lastRow =recievedMessage.value2;
	
	for(int i=1;i<=totalGenerations;i++){
		int type;
		RecvMsg(threadId, &recievedMessage);
		type = recievedMessage.type;
		if(type==STOP){
			break;			
			//exit(0);
		}
		int allDead=1;
		int steadyState=1;
		for(int row=firstRow;row<lastRow;row++){
			for(int col=0;col<totalColumns;col++){
				int neighbors = countNeighbors(row, col, (i-1)%2);
				int survive = -1;
				if(neighbors==3){
					survive=1;
				}else if(neighbors<2 || neighbors>3){
					survive=0;
				}
				if(i%2==1){
					if(survive==-1){
						oddGeneration[row][col]=evenGeneration[row][col];
					}else{
						oddGeneration[row][col]=survive;
					}
					if(oddGeneration[row][col]!=0){
						allDead=0;
					}
				}else{
					if(survive==-1){
						evenGeneration[row][col]=oddGeneration[row][col];
					}else{
						evenGeneration[row][col]=survive;
					}
					if(evenGeneration[row][col]!=0){
						allDead=0;
					}
				}
				if(oddGeneration[row][col]!=evenGeneration[row][col]){
					steadyState=0;
				}
			}
		}

		struct msg* genDone = new struct msg;
		genDone->iSender = threadId;
		genDone->type=GENDONE;
		genDone->value1=allDead;
		genDone->value2=steadyState;
		SendMsg(0, genDone);
	}		

	struct msg* returnMessage=new struct msg;
	returnMessage->iSender = threadId;
	returnMessage->type=ALLDONE;
	SendMsg(0, returnMessage);
}

/*
 * main
 * the main method of the program
 * processes command line, creates threads, returns results
 */
int main(int argc, char* argv[]){
	int totalThreads=0;
	totalGenerations=0;
	int printFlag = 0;
	int inputFlag = 0;
	string exitReason = "max generations were reached";
	if(argc<4){
		cout << "Please enter the correct number of arguments\n";
		cout << "It should be the form \"./life #threads filename #generations 'print' 'input'\"\n";
		exit(1);
	}
	if(atoi(argv[1]) < 1){
		cout << "Please enter a positive number of threads\n";
		exit(1);
	}else{
		totalThreads = atoi(argv[1]);
	}
	if(atoi(argv[3]) < 1){
		cout << "Please enter a positive number of generations\n";
		exit(1);
	}else{
		totalGenerations = atoi(argv[3]);
	}
	if(argc>4){
		if(strcmp(argv[4],"y")==0){
			printFlag=1;
		}
	}
	if(argc>5){
		if(strcmp(argv[5],"y")==0){
			inputFlag=1;
		}
	}
	const char* inputFileName=argv[2];
	
	ifstream inputFile;
	string lineReader;

	inputFile.open(inputFileName);
	totalRows=0;
	totalColumns=0;
	if(!inputFile){
		cerr << "Error opening file, please check the file name\n";
		exit(1);
	}
	while(getline(inputFile, lineReader)){
		istringstream ss (lineReader);
		int count =0;
		int a = 0;
		while(ss>>a){
			count++;
		}
		if(totalColumns==0){
			totalColumns = count;
		}else if(totalColumns != count && count != 0){
			cerr << "The grid in the input file is not a rectangle\n";
			exit(1);
		}
		if(count != 0){
			totalRows++;
		}
	}
	inputFile.close();

	if(totalRows==0 || totalColumns == 0){
		cerr << "The grid is too small\n";
		exit(1);
	}
	if(totalRows>MAXGRID || totalColumns>MAXGRID){
		cerr << "Grid too big!!!!\n";
		exit(1);
	}
	evenGeneration = new int* [totalRows];
	oddGeneration = new int* [totalRows];
	for(int i=0;i<totalRows;i++){
		evenGeneration[i]=new int[totalColumns];
		oddGeneration[i]=new int[totalColumns];
	}

	inputFile.open(inputFileName);
	for(int i=0; i<totalRows;i++){
		for(int j=0;j<totalColumns;j++){
			int a;
			inputFile >> a;
			evenGeneration[i][j]=a;
			oddGeneration[i][j]=0;
		}
	}
	inputFile.close();
	
	if(totalThreads > totalRows){
		totalThreads = totalRows;
	}
	
	//add 1 for the parent thread
	mailboxes = new struct msg * [totalThreads+1];
	sendSemaphore = new sem_t[totalThreads+1];
	recvSemaphore = new sem_t[totalThreads+1];
	for(int i = 0; i<totalThreads+1; i++){
		sem_init(&sendSemaphore[i], 0, 1);
		sem_init(&recvSemaphore[i], 0, 0);
	}
	
	pthread_t children [totalThreads];
	
	int rangePerThread = totalRows/totalThreads;
	int remainder = totalRows % totalThreads;
	int counter = 0;
	for(int i=0; i<totalThreads; i++){
		int startRange = counter;
		int endRange = startRange + rangePerThread;
		if(remainder > 0){
			endRange++;
			remainder--;
		}
		if(pthread_create(&children[i], NULL, childTask, (void *)(i+1)) != 0){
			cerr << "Thread creation failed\n";
			exit(1);
		}
		struct msg* sendMessage = new struct msg;
		sendMessage->iSender = 0;
		sendMessage->type = RANGE;
		sendMessage->value1 = startRange;
		sendMessage->value2 = endRange;
		SendMsg(i+1, sendMessage);
		
		counter=endRange;
	}

	int threadsDone = 0;
	int lastGeneration = 0;
	for(int i=0;i<totalGenerations;i++){
		if(printFlag || i==0){
			if(i==0){
				cout<<"Starting position:\n";
			}else{
				cout<<"Generation "<< i << ":\n";
			}
			printGrid(i%2);
			cout<<"\n";
		}
		if(inputFlag && printFlag){
			cin.get();
		}
		
		for(int thread=0;thread<totalThreads;thread++){
			struct msg* goTime = new msg;
			goTime->iSender=0;
			goTime->type = GO;
			SendMsg(thread+1, goTime);
		}
		
		int allDead=1;
		int steadyState=1;	
		for(int thread=0;thread<totalThreads;thread++){
			struct msg recv;
			int type;	
			RecvMsg(0,&recv);
			type=recv.type;
			if(type==ALLDONE){
				threadsDone++;
			}
			if(recv.value1==0){
				allDead=0;
			}
			if(recv.value2==0){
				steadyState=0;
			}
		}
		lastGeneration = i+1;
		if(allDead == 1){
			for(int thread=0;thread<totalThreads;thread++){
				struct msg* stopTime = new msg;
				stopTime->iSender = 0;
				stopTime->type=STOP;
				SendMsg(thread+1, stopTime);
			}
			exitReason = "all cells are dead :(";
			break;
		}
		if(steadyState==1){
			for(int thread=0;thread<totalThreads;thread++){
				struct msg* stopTime = new msg;
				stopTime->iSender = 0;
				stopTime->type=STOP;
				SendMsg(thread+1, stopTime);
			}
			exitReason = "a repeating state has been reached";
			lastGeneration = i;
			break;
		}
	}


	for(int i=0;i<totalThreads-threadsDone;i++){
		struct msg recievedMessage;
		int type;
		while(type!=ALLDONE){
			RecvMsg(0,&recievedMessage);
			type=recievedMessage.type;
		}
	}

	cout<<"The game ended because "<<exitReason<<" after "<<lastGeneration<<" generations with:\n";
	printGrid(lastGeneration%2);

	exit(0);
}










