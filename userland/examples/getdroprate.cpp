/* g++ getdroprate.cpp -o getdroprate -std=c++11 -lpthread -lmysqlclient -L/usr/lib64/mysql*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/msg.h>
#include<sys/ipc.h>
#include<string>
#include<fstream>
#include<iostream>
#include <errno.h>
extern char *optarg;
using namespace std;
struct mymesg{
    long int mtype;
    double droprate;
    char nowbpffilter[255];
    char mtext[5120];
    int id;
    key_t key;
};
struct mymesg pfmsg;
key_t key = ftok("/tmp",2);
time_t t1=time(NULL);
time_t t0=time(NULL);
ofstream dropfile("droprate.txt");
fstream bpfs;
int id = msgget(key,0666|IPC_CREAT);
int main(int argc, char* argv[])
{  
    if(id == -1){
        printf("open msg error \n");
        return 0;
    }
    pfmsg.id=id;
    pfmsg.key=key;
    pfmsg.mtype=1;
    while(1){
        printf("key:%x id:%d\n",key,id);
        if(msgrcv(id,(void *)&pfmsg,sizeof(double),pfmsg.mtype,0) < 0){
            printf("1 receive droprate error %d\n",errno);
        }
        else{
            double droprate=pfmsg.droprate;
            time_t t2=time(NULL);
            if(t2!=t1){
                dropfile<<droprate<<endl;
                printf("1 droprate:%f\n",droprate);
                t1=t2;
            }
            if(t2-t0>=120){
                dropfile.close();
                dropfile.open("droprate.txt");
                t0=t2;
            }
        }
    }
    return 0;
}