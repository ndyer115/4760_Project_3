//Author: Nolan Dyer
//Date: 3/14/23
#include <iostream>
#include <iomanip>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <fstream>
using namespace std;

int nanoSeconds = 0;//global variables for clock
int seconds = 0;

struct pt {//process table struct
int occ;
pid_t pid;
int startSec;
int startNano;
} procTable;

struct pt procInfo[20];

struct msg_buffer {//message buffer to be passed to worker processes
    long msg_type = 1;
    int msgSec;
    int msgNano;
} message;

void sharedClock(bool inc) {//each loop of oss increments nanoseconds by 950 and updates seconds and nanoseconds in struct
if (inc) 
  nanoSeconds = nanoSeconds + 950;

if (nanoSeconds > 1000000000) {
  seconds++;
  nanoSeconds = 0;
}
}


int main(int argc, char** argv) {

bool help = false;
int proc = 1, simul = 1, runSec = 1, runNano = 0;
char logFile[20] = "Logfile";

//obtains command line arguments
for (int i = 0; i < argc; i++) {
  if (strcmp(argv[i], "-h") == 0 || argc == 1) 
    help = true;
  if (strcmp(argv[i], "-n") == 0 && i != argc-1 && argv[i+1])
    proc = atoi(argv[i+1]);
  
  if (strcmp(argv[i], "-s") == 0 && i != argc-1 && argv[i+1])
    simul = atoi(argv[i+1]);
  
  if (strcmp(argv[i], "-f") == 0 && i != argc-1 && argv[i+1])
    strncpy(logFile, argv[i+1], sizeof(logFile));
  
  if (strcmp(argv[i], "-t") == 0 && i != argc-1 && argv[i+1]) {
    runSec = atoi(argv[i+1]);
    runNano = atoi(argv[i+2]);
  }
} 

fstream outputFile;
outputFile.open(logFile, ios::out);//opens logfile so that it can be written to


if (help) {//displays help message and terminates program
  cout<<"Help Menu"<<endl;
  cout<<"-h: brings up help menu and terminates program"<<endl;
  cout<<"-n: total number of processes to be executed"<<endl;
  cout<<"-s: limit for how many processes can run simultaneously"<<endl;
  cout<<"-t: number of iterations each worker process will perform"<<endl;
  cout<<"For example, to run the program with 10 worker processes,"<<endl;
  cout<<"a limit of 2 simultaneous processes, and 3 iterations per process,"<<endl;
  cout<<"the command would be: ./oss.out -n 10 -s 2 -t 3"<<endl;
  exit(0);
}

//check to prevent too many processes from running at the same time
if (simul > 15)
  simul = 15;

//prevents incorrect number of processes being launched in case that simul > proc
if (simul > proc)
simul = proc;

const char* args[] = {"./worker.out", NULL};

//program launches the number of simultaneous processes and continues to launch processes as more terminate
int curProcCount = 0;
int totProcCount = 0;
int status;
bool newProc = false;

int shmidNano = shmget(2000, 512, 0644 | IPC_CREAT);//gets shared memory space or creates it if it does not exist
int shmidSec = shmget(2001, 512, 0644 | IPC_CREAT);
int *blockNano = (int*) shmat(shmidNano, NULL, 0);
int *blockSec = (int*) shmat(shmidSec, NULL, 0);

int msgid = msgget(3000, 0666 | IPC_CREAT);//creates ID for msgsnd

while (true) {
  if (curProcCount < simul && totProcCount != proc) {//if the current number of processes running is less than the simul limit and the total number has not reached the desired amount of processes, fork()
    curProcCount++;
    totProcCount++;
    newProc = true;
    procTable.pid = fork();
  }
  
  if (procTable.pid == 0) {
    srand(getpid());
    execlp(args[0], args[0], args[1]);//executes worker on child process, exits with error if execlp line is missed
    cout<<"Exec failed, terminating program"<<endl;
    exit(1);
  } 
  
  if (newProc) {//generates random time limit values, sends them to worker, and tracks process table info 
    newProc = false;
    message.msgSec = 1+(rand()%runSec);
    message.msgNano = 1+(rand()%runNano);
    msgsnd(msgid, &message, sizeof(message), 0);
    procInfo[totProcCount-1].occ = 1;
    procInfo[totProcCount-1].pid = procTable.pid;
    procInfo[totProcCount-1].startSec = seconds;
    procInfo[totProcCount-1].startNano = nanoSeconds;
  }
  
  pid_t childPid = waitpid(-1, &status, WNOHANG);
  
  if (WIFEXITED(status) && childPid > 0) {//detects if worker process has completed, changes occupied for process entry to false, and decrements current number of running processes
    for (int i = 0; i < totProcCount; i++)
      if (procInfo[i].pid == childPid)
        procInfo[i].occ = 0;
    curProcCount--;
  }
  
  sharedClock(1);//increments clock function
  *blockNano = (int)nanoSeconds;
  *blockSec = (int)seconds;
  
  if (nanoSeconds == 500000200 || nanoSeconds == 0) {//outputs process table every half second and puts process table data into logfile
    cout<<"OSS PID: "<<getpid()<<" SysClockS: "<<seconds<<" SysClockN: "<<nanoSeconds<<endl;
    cout<<"Process Table: "<<endl;
    cout<<"Entry Occupied PID   StartS StartN"<<endl;
    outputFile<<"OSS PID: "<<getpid()<<" SysClockS: "<<seconds<<" SysClockN: "<<nanoSeconds<<endl;
    outputFile<<"Process Table: "<<endl;
    outputFile<<"Entry Occupied PID   StartS StartN"<<endl;
    for (int i = 0; i < totProcCount; i++) {
       cout<<left<<setw(6)<<i<<setw(9)<<procInfo[i].occ<<setw(6)<<procInfo[i].pid<<setw(7)<<procInfo[i].startSec<<procInfo[i].startNano<<endl;
       outputFile<<left<<setw(6)<<i<<setw(9)<<procInfo[i].occ<<setw(6)<<procInfo[i].pid<<setw(7)<<procInfo[i].startSec<<procInfo[i].startNano<<endl;
    }
    cout<<endl;
  }
  
  if (totProcCount == proc && curProcCount == 0)//exits loop if total number of processes have finished running
    break;
    
  if (seconds > 60) {//exits if max time limit is reached
    cout<<"Time limit reach, terminating all processes"<<endl;
    shmdt(blockNano);
    shmdt(blockSec);
    shmctl(shmidNano,IPC_RMID,NULL);
    shmctl(shmidSec,IPC_RMID,NULL);
    msgctl(msgid, IPC_RMID, NULL);
    outputFile.close();
    exit(1);
  }
}

wait(0);
cout<<endl<<"All worker processes have completed."<<endl;

shmdt(blockNano);//closes shared memory, message queue, and logfile before terminating
shmdt(blockSec);
shmctl(shmidNano,IPC_RMID,NULL);
shmctl(shmidSec,IPC_RMID,NULL);
msgctl(msgid, IPC_RMID, NULL);
outputFile.close();
return 0;
}

