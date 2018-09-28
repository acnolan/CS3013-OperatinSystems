/*
 * addem.cpp
 * Andrew Nolan  - acnolan
 * Part 1 of Homework 3
 */

#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <iostream>
#include <semaphore.h>

using namespace std;

//define the message types
#define RANGE 1
#define ALLDONE 2

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
	struct msg recievedMessage;
	RecvMsg(threadId, &recievedMessage);
	int sum=0;
	for(int i=recievedMessage.value1; i<recievedMessage.value2; i++){
		sum += i;
	}
	struct msg * returnMessage=new struct msg;
	returnMessage->iSender = threadId;
	returnMessage->type=ALLDONE;
	returnMessage->value1=sum;
	SendMsg(0, returnMessage);
}

/*
 * main
 * the main method of the program
 * processes command line, creates threads, returns results
 */
int main(int argc, char* argv[]){
	int totalThreads = 0;
	int countTo = 0;
	int tooManyThreadsFlag = 0;
	if(argc<3){
		cout << "Please enter the correct number of arguments\n";
		cout << "It should be the form './addem #threads #count'\n";
		exit(1);
	}
	if(atoi(argv[1]) < 1){
		cout << "Please enter a positive number of threads\n";
		exit(1);
	}else{
		totalThreads = atoi(argv[1]);
	}
	if(atoi(argv[2]) < 1){
		cout << "Please enter a positive number to count to\n";
		exit(1);
	}else{
		countTo = atoi(argv[2]);
	}
	if(totalThreads > countTo){
		totalThreads = countTo;
		tooManyThreadsFlag=1;
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
	
	int rangePerThread = countTo/totalThreads;
	int remainder = countTo % totalThreads;
	int counter = 1;
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
	int sum = 0;
	for(int i=0;i<totalThreads;i++){
		struct msg recievedMessage;
		RecvMsg(0, &recievedMessage);
		sum += recievedMessage.value1;
	}
	if(tooManyThreadsFlag){
		cout<<"The total for 1 to "<<countTo<<" using "<<argv[1]<< " threads (reduced to " <<totalThreads<<" because of too many threads) is " <<sum<<".\n";
	}else{
		cout<<"The total for 1 to " <<countTo<< " using "<<totalThreads<<" threads is " <<sum<<".\n";
	}

	exit(0);
}
