#include <iostream>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <cstdint>
using namespace std;

struct msg_buffer {
    long mesg_type;
    int msgSec;
    int msgNano;
} message;

int main (int argc, char** argv) {
int shmidNano = shmget(2000, 512, 0644);//creates IDs for shared memory
int shmidSec = shmget(2001, 512, 0644);
int *blockNano = (int*) shmat(shmidNano, NULL, 0);
int *blockSec = (int*) shmat(shmidSec, NULL, 0);

int msgid = msgget(3000, 0666 | IPC_CREAT);//creates ID for msgrcv
msgrcv(msgid, &message, sizeof(message), 1, 0);//waits until message is recieved from oss

int *startSec = blockSec;
int *startNano = blockNano;
int targetSec = *startSec + message.msgSec;//calculates when worker should terminate
int targetNano = *startNano + message.msgNano;

cout<<"WORKER PID:"<<getpid()<<" PPID:"<<getppid()<<" Received seconds from oss: "<<message.msgSec<<" Received Nano from oss: "<<message.msgNano<<" TermTimeS: "<<targetSec<<" TermTimeNano: "<<targetNano<<endl<<"--Received message"<<endl;

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


shmdt(blockNano);//detaches from shared memory space
shmdt(blockSec);

return 0;
}

