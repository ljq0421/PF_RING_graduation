/* g++ getdownbpf.cpp -o getdownbpf -std=c++11*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/msg.h>
#include<sys/ipc.h>
#include<map>
#include<string>
#include<unistd.h>
#include<fstream>
#include<iostream>
#include <errno.h>
#include<algorithm>
#include<vector>
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
fstream bpfs;
int id = msgget(key,0666|IPC_CREAT);
int main(int argc, char* argv[])
{  
	if(id == -1){
		printf("open msg error \n");
		return 0;
	}
    while(1){
        printf("key:%x id:%d\n",key,id);
        pfmsg.mtype=2;
        if(msgrcv(id,(void *)&pfmsg,255,pfmsg.mtype,IPC_NOWAIT) < 0){
            printf("2 msgrcv mtype=2 error\n");
        }
        else{
            printf("2 need to change bpf,now is :%s\n",pfmsg.nowbpffilter);
            bpfs.open("bpfs.txt",ios::in);
            vector<string> bpfvec;
            string line;
            while(getline(bpfs,line)){
                bpfvec.push_back(line);
            }
            bpfs.close();
            sort(bpfvec.begin(),bpfvec.end(),[](string a, string b){return a.size() < b.size();});
            //按照长短排优先级，越短的过滤强度越低，越靠前，mtype=2，向下调，需要过滤强度高一些
            for(int i=0;i<bpfvec.size();i++){
                if(bpfvec[i]==pfmsg.nowbpffilter){
                    if(i+1<bpfvec.size()){
                        strcpy(pfmsg.nowbpffilter,bpfvec[i+1].c_str());
                        int ret=msgsnd(pfmsg.id,(void *)&pfmsg,255,IPC_NOWAIT);
                        if(ret<0) printf("2 send back bpf msg error %d\n",errno);
                        else{
                            printf("2 next %s\n",bpfvec[i+1].c_str());
                            swap(bpfvec[i+1],bpfvec[0]);
                        }
                    } //发送回去下一个，调整bpfs.txt的文件排序，重写吧
                    else{
                        int ret=msgsnd(pfmsg.id,(void *)&pfmsg,255,IPC_NOWAIT);
                        if(ret<0) printf("send back bpf msg error %d\n",errno);
                        else{
                            printf("2 next my self %s\n",bpfvec[i].c_str());
                            swap(bpfvec[i],bpfvec[0]);
                        }
                    } //把自己发送回去
                }
            }
            bpfs.open("bpfs.txt",ios::out);
            for(int i=0;i<bpfvec.size();i++){
                bpfs<<bpfvec[i]<<endl;
            }
            bpfs.close();
        }
    }
	return 0;
}