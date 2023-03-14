#include <iostream>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <cstdint>
using namespace std;

struct mesg_buffer {
    long mesg_type;
    int msgSec;
    int msgNano;
} message;

int main (int argc, char** argv) {
int shmidNano = shmget(2000, 512, 0644);
int shmidSec = shmget(2001, 512, 0644);
int *blockNano = (int*) shmat(shmidNano, NULL, 0);
int *blockSec = (int*) shmat(shmidSec, NULL, 0);

int msgid = msgget(3000, 0666 | IPC_CREAT);//creates ID for msgrcv
msgrcv(msgid, &message, sizeof(message), 1, 0);

cout<<"seconds: "<<message.msgSec<<" nano: "<<message.msgNano<<endl;

int *startSec = blockSec;
int *startNano = blockNano;
int targetSec = *startSec + stoi(argv[1]);
int targetNano = *startNano + stoi(argv[2]);

cout<<"WORKER PID:"<<getpid()<<" PPID:"<<getppid()<<" SysClockS: "<<*blockSec<<" SysClockNano: "<<*blockNano<<" TermTimeS: "<<targetSec<<" TermTimeNano: "<<targetNano<<endl<<"--Just Starting"<<endl;

int time = *startSec;
int passed = 0;
while (true) {
  if (*blockSec > time) {
    time++;
    passed++;
    cout<<"WORKER PID:"<<getpid()<<" PPID:"<<getppid()<<" SysClockS: "<<*blockSec<<" SysClockNano: "<<*blockNano<<" TermTimeS: "<<targetSec<<" TermTimeNano: "<<targetNano<<endl<<"--"<<passed<<" seconds have passed since starting"<<endl;
  }
  
  if (*blockSec > targetSec && *blockNano > targetNano) {
    cout<<"WORKER PID:"<<getpid()<<" PPID:"<<getppid()<<" SysClockS: "<<*blockSec<<" SysClockNano: "<<*blockNano<<" TermTimeS: "<<targetSec<<" TermTimeNano: "<<targetNano<<endl<<"--Terminating"<<endl;
    
    break;
  }
}


shmdt(blockNano);
shmdt(blockSec);

return 0;
}

