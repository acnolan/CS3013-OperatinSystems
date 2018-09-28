// doit.cpp
// Andrew Nolan
// acnolan@wpi.edu


#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <unistd.h>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>

using namespace std;


/**
 * Struct for storing a background process's data
 */
struct process{
	int pid;
	string args;
	struct timeval startTime;
};

/**
 * struct to store the total usage data
 */
struct rusage total;

/**
 * forkAndRun
 * creates the new process with the fork and executes the desired command
 * argc - the number of arguments
 * args - list of strings containing the arguments for the command
 */
int forkAndRun(int argc, char* args[]){
	int pid;//the process id
	if((pid = fork()) < 0){
		cerr << "Fork error\n";
		exit(1);
	}else if(pid==0){
		for(int i=0;i<argc-1;i++){
			args[i]=args[i+1];
		}
		args[argc-1]=0;
		execvp(args[0], args);		
		cerr<<"execvp error \n";
             	exit(1);				
	}else{
		return pid;
	}
}

/**
 * printStatistics
 * prints the statistics of the run
 */
void printStatistics(){
	struct rusage stats;
	struct timeval startTime;
	struct timeval endTime;	
	gettimeofday(&startTime, NULL);
	int status;
	waitpid(-1, &status, 0);
	getrusage(RUSAGE_CHILDREN, &stats);
	gettimeofday(&endTime, NULL);
	cout << "\nStatistics:\n";
	cout <<"CPU System Time: " <<(stats.ru_stime.tv_sec *1000.0 +stats.ru_stime.tv_usec/1000.0) - (total.ru_stime.tv_sec * 1000.0 + total.ru_stime.tv_usec/1000.0)<<" milliseconds\n";
	cout <<"CPU User Time: " <<(stats.ru_utime.tv_sec *1000.0 +stats.ru_utime.tv_usec/1000.0)-(total.ru_utime.tv_sec * 1000.0 + total.ru_utime.tv_usec/1000.0)<<" milliseconds\n";
	cout <<"Elapsed wall clock time: " <<(endTime.tv_sec - startTime.tv_sec)*1000.0 + (endTime.tv_usec - startTime.tv_usec)/1000.0 << " milliseconds\n";
	cout <<"Number of times process was preempted involuntarily: " << stats.ru_nivcsw -total.ru_nivcsw <<"\n";
	cout <<"Number of times process gave up the CPU voluntarily: " << stats.ru_nvcsw -total.ru_nvcsw<<"\n";
	cout <<"Number of major page faults: " << stats.ru_majflt -total.ru_majflt<< "\n";
	cout <<"Number of minor page faults: " << stats.ru_minflt -total.ru_minflt<< "\n";
	cout <<"Maximum resident set size used: " <<stats.ru_maxrss<< " kilobytes\n";
	
	stats.ru_maxrss=0;
	total = stats;
}

/**
 * printStatistics (overload)
 * prints the statistics of the run for a background process
 * @param startTime is the startTime of the process
 */
void printStatistics(struct timeval startTime){
	struct rusage stats;
	struct timeval endTime;
	//gettimeofday(&startTime, NULL);
	int status;
	//waitpid(-1, &status, 0); //?
	getrusage(RUSAGE_CHILDREN, &stats);
	gettimeofday(&endTime, NULL);
	cout << "\nStatistics:\n";
	cout <<"CPU System Time: " <<(stats.ru_stime.tv_sec *1000.0 +stats.ru_stime.tv_usec/1000.0)-(total.ru_stime.tv_sec * 1000.0 + total.ru_stime.tv_usec/1000.0)<<" milliseconds\n";
	cout <<"CPU User Time: " <<(stats.ru_utime.tv_sec *1000.0 +stats.ru_utime.tv_usec/1000.0)-(total.ru_utime.tv_sec*1000.0 + stats.ru_utime.tv_usec/1000.0)<<" milliseconds\n";
	cout <<"Elapsed wall clock time: " <<(endTime.tv_sec - startTime.tv_sec)*1000.0 + (endTime.tv_usec - startTime.tv_usec)/1000.0 << " milliseconds\n";
	cout <<"Number of times process was preempted involuntarily: " << stats.ru_nivcsw -total.ru_nivcsw <<"\n";
	cout <<"Number of times process gave up the CPU voluntarily: " << stats.ru_nvcsw -total.ru_nvcsw <<"\n";
	cout <<"Number of major page faults: " << stats.ru_majflt -total.ru_majflt << "\n";
	cout <<"Number of minor page faults: " << stats.ru_minflt - total.ru_minflt << "\n";
	cout <<"Maximum resident set size used: " <<stats.ru_maxrss << " kilobytes\n";

	total = stats;
}


/**
 * shell
 * runs the shell version of the program
 */
void shell(){
	string prompt = "==>";
	bool shellMode = true;
	string argString;
	vector<process> backgroundProcesses;
	bool background;
	while(shellMode){
		background=false;
		for(int i=0;i<backgroundProcesses.size();i++){
			int storage;
			int result = waitpid(backgroundProcesses.at(i).pid, &storage, WNOHANG);
			if(result != 0){
				cout << "[" <<i+1<<"] " <<backgroundProcesses[i].pid<<"  Completed\n";
				printStatistics(backgroundProcesses.at(i).startTime);
				backgroundProcesses.erase(backgroundProcesses.begin()+i);
			}
		}
		cout << prompt;
		getline(cin, argString);
		if(!cin){
			if(cin.eof()){
				cout << "Detected EOF, exiting... \n";
				//argString="exit";
				exit(0);
			}
		}
		if(argString.length() > 128){
			cout <<"input cannot be more than 128 characters\n";
			argString="";
		}
		istringstream argStream (argString);
		string arg;
		vector<string> argList;
		while(argStream >> arg){
			argList.push_back(arg);
		}
		if(argList.size()>0){
			if(argList[argList.size()-1] == "&"){
				background=true;
				argList.erase(argList.begin() + argList.size()-1);
			}
		}
		if(argList.size()==0){
			//do nothing so user recieves prompt again
		}else if(argList[0] == "exit"){
			//exit
			if(backgroundProcesses.size() > 0 ){
				cout << "Please wait until all background tasks are finished running to exit\n";
			}else{
				exit(0);
			}
		}else if(argList[0]=="jobs"){
			//list the background tasks
			if(backgroundProcesses.size() == 0){
				cout << "No background processes active\n";
			}else{
				for(int i=0;i<backgroundProcesses.size();i++){
					cout << "[" << i+1 << "] " << backgroundProcesses[i].pid <<" "<<backgroundProcesses[i].args << "\n";
				}
			}
		}else if(argList[0]=="cd"){
			//change directory
			if(argList.size()==1){
				if(chdir(getenv("HOME"))<0){
					cerr << "Error changing directory, make sure the directory is 							 named correctly";
				}else{
					cout << "Working directory has been changed to: home\n";
				}
			}else{
				if(chdir(argList[1].c_str())<0){
					cerr << "Error changing directory, make sure the directory is 							 named correctly";
				}else{
					cout << "Working directory has been changed to: " 							<< argList[1].c_str() <<"\n";
				}
			}	
			
		}else if(argList[0]=="set" && argList[1]=="prompt" && argList[2]=="="){
			//change prompt string
			cout << "The prompt has been changed from '" << prompt << "' to '" << 					 argList[3] << "'\n";
			prompt = argList[3];			
		}else{
			//run the process and return stats
			char* argList2[argList.size()+1];
			argList2[0]=(char*)"./doit";
			for(int i=0; i<argList.size(); i++){
				argList2[i+1] = (char*)argList[i].c_str();
			}
			if(background == true){
				struct timeval start;
				gettimeofday(&start, NULL);
				process p = {forkAndRun(argList.size()+1, argList2), argString, start};
				backgroundProcesses.push_back(p);
				cout << "[" <<backgroundProcesses.size()<<"] " <<backgroundProcesses.back().pid << " " << backgroundProcesses.back().args << " has started\n";
			}else{
				cout << "Executing Process...\n";
				forkAndRun(argList.size()+1, argList2);
				printStatistics();
			}
		}
		argList.clear();
	}
}

/**
 * main
 * the main method for the program
 * @param argc - number of arguments
 * @param argv - array of strings representing the arguments
 */
int main(int argc, char* argv[]){
	getrusage(RUSAGE_CHILDREN, &total);
	if(argc==1){
		cout <<"ENTERING SHELL MODE\n";
		shell();
		exit(1);
	}else if(argc>1){
		forkAndRun(argc, argv);
		printStatistics();
	}	
	exit(0);
}
