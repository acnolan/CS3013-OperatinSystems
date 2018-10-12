/*
 * proj4.cpp
 * Andrew Nolan - acnolan
 */

#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <iostream>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

using namespace std;

//define the message types
#define RANGE 1
#define ALLDONE 2

//define maxes for this program
#define MAXTHREADS 16
#define MAXCHUNK 8192

char * filemap;

int strIndex; //an index to keep track of where we are in the search string during read

/*
 * struct msg
 * a struct to store message data
 */
struct msg{
	int iSender;  	//sender of the message
	int type;	//type of message
	int value1;	//first value
	int value2;	//second value
	char* str;	//the string being searched for
	int strLength;	//the length of the string
};

//The mailbox storage struct
struct msg ** mailboxes;

//the arrays of semaphores for sending and recieving
sem_t * sendSemaphore;
sem_t * recvSemaphore;

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
 * childTask
 * This is the task that the child threads complete before sending a message back to the parent
 * @param Id - the mailbox id of the child thread
 */
void* childTask(void* Id){
	int threadId = (int)Id;
	int stringCount=0;
	struct msg recievedMessage;
	RecvMsg(threadId, &recievedMessage);
	char* searchString = recievedMessage.str;
	int strLength = recievedMessage.strLength;
	int badFlag=0;//a flag that becomes true when the string is not found
	for(int i=recievedMessage.value1; i<recievedMessage.value2; i++){
		badFlag=0;
		for(int j=0;j<strLength;j++){
			if(filemap[i+j] != searchString[j]){
				badFlag=1;
				j=strLength+1;
			}
		}
		if(!badFlag){
			stringCount++;
		}
	}


	struct msg * returnMessage=new struct msg;
	returnMessage->iSender = threadId;
	returnMessage->type=ALLDONE;
	returnMessage->value1=stringCount;
	SendMsg(0, returnMessage);
}

/*
 * mapSearch
 * searches for the given string using a memory map
 * Really calls child threads to search using a memory map
 * @param file: the file we are reading
 * @param fileSize: the size of the file in bytes
 * @param totalThreads: the number of threads being used (just 1 if standard mmap)
 * @param searchString: the string we are searching for
 * @param strLength: the length of the string we are searching for
 * @return the number of occurneces of searchString in file
 */
int mapSearch(int file, int fileSize, int totalThreads, char* searchString, int strLength){
	int stringCount=0;
	filemap = (char*)mmap(NULL, fileSize, PROT_READ, MAP_SHARED, file, 0);
	if(filemap==(char*)-1){
		cerr<<"Error mapping file.\n";
		exit(1);
	}
	mailboxes = new struct msg * [totalThreads+1];
	sendSemaphore = new sem_t[totalThreads+1];
	recvSemaphore = new sem_t[totalThreads+1];
	for(int i = 0; i<totalThreads+1; i++){
		sem_init(&sendSemaphore[i], 0, 1);
		sem_init(&recvSemaphore[i], 0, 0);
	}
	
	pthread_t children [totalThreads];
	
	int rangePerThread = fileSize/totalThreads;
	int remainder = fileSize % totalThreads;
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
		sendMessage->str=searchString;
		sendMessage->strLength=strLength;
		SendMsg(i+1, sendMessage);
		
		counter=endRange;
	}
	for(int i=0;i<totalThreads;i++){
		struct msg recievedMessage;
		RecvMsg(0, &recievedMessage);
		stringCount += recievedMessage.value1;
	}
	
	for(int i=0;i<totalThreads;i++){
		pthread_join(children[i],NULL);
	}
	delete mailboxes;
	return stringCount;
}

/*
 * readSearch
 * searches for the given string using the read system command
 * @param file: the file we are reading
 * @param chunkSize: the size of the chunk we are reading at a time
 * @param searchString: the string we are searching for
 * @param strLength: the length of the string we are searching for
 * @return the number of occurences of searchString in file
 */
int readSearch(int file, int chunkSize, char* searchString, int strLength){
	int stringCount=0;
	char buff[chunkSize];
	int readReturn=0;
	int strPointer=0;
	while((readReturn = read(file, buff, chunkSize))>0){
		for(int i=0;i<readReturn;i++){
			if(buff[i]==searchString[strPointer]){
				strPointer++;
			}else{
				strPointer=0;
			}
			if(strPointer==strLength){
				stringCount++;
			}
		}
	}
	return stringCount;
}

/*
 * main
 * the main method of the program
 * processes command line and returns resultsf
 */
int main(int argc, char* argv[]){
	int totalThreads = 1;
	char* searchString;
	int chunkSize = 1024;
	int mmapFlag = 0;
	if(argc<3){
		cout << "Please enter the correct number of arguments\n";
		cout << "It should be the form './proj4 srcfile searchstring [size|mmap|p#]'\n";
		exit(1);
	}
	int file;
	if((file = open(argv[1], O_RDONLY)) < 0){
		cerr << "Error opening file, please make sure the file name is written correctly\n";
		exit(1);
	}
	searchString=argv[2];
	if(argc>3){
		if(strcmp(argv[3],"mmap")==0){
			mmapFlag=1;
		}else if(argv[3][0]=='p'){
			mmapFlag=1;
			char* pString = argv[3];
			pString++;//get rid of the p
			totalThreads = atoi(pString);
		}else{
			chunkSize = atoi(argv[3]);
		}
	}
	if(totalThreads<=0){
		cerr << "Please enter a positive number of threads\n";
		exit(1);	
	}else if(totalThreads>MAXTHREADS){
		cout << "The Max Thread Limit is " << MAXTHREADS <<", setting thread count to "<<MAXTHREADS<<"\n";
		totalThreads=MAXTHREADS;
	}

	struct stat stats;
	if(fstat(file, &stats)<0){
		cerr<<"Error getting file size\n";
		exit(1);
	}
	int stringCount=0;
	if(chunkSize>MAXCHUNK){
		cout << "The chunk size limit is " << MAXCHUNK <<", setting chunk size to "<<MAXCHUNK<<"\n";
		chunkSize = MAXCHUNK;
	}
	if(chunkSize<1){
		cerr << "Please enter a positive chunk size\n";
		exit(1);
	}
	int strLength = strlen(searchString);
	if(mmapFlag==1){
		//cout<<"here";
		stringCount = mapSearch(file, stats.st_size, totalThreads, searchString, strLength);
	}else{
		//cout<<"there";
		stringCount = readSearch(file, chunkSize, searchString, strLength);
	}
	string searchType="the read system call.";
	if(totalThreads>1&&mmapFlag){
		searchType="memory map with multiple threads";
	}else if(totalThreads==1 && mmapFlag){
		searchType="memory map.";
	}
	cout << "Using "<<searchType<<"\n";
	cout<<"File size: "<<stats.st_size<<" bytes.\n";
	cout<<"Occurrences of the string \""<<searchString<<"\": "<<stringCount<<"\n";
	close(file);
	
	exit(0);
}
		
