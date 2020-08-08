/* g++ getdroprate.cpp -o getdroprate -std=c++11*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/msg.h>
#include<sys/ipc.h>
#include<mysql/mysql.h>
#include<map>
#include<string>
#include<unistd.h>
#include<fstream>
#include<iostream>
#include <errno.h>
#include<algorithm>
#include<vector>
#include<thread>
extern char *optarg;
using namespace std;
struct mymesg{
	long int mtype;
	double droprate;
    char nowbpffilter[255];
    int id;
    key_t key;
};
struct mymesg pfmsg;
key_t key = ftok("/tmp",101);
time_t t1=time(NULL);
time_t t0=time(NULL);
ofstream dropfile("droprate.txt");
fstream bpfs;
int id = msgget(key,0666|IPC_CREAT);
void thread1(){
    while(1){
        pfmsg.mtype=1;
        if(msgrcv(id,(void *)&pfmsg,sizeof(double),pfmsg.mtype,0) < 0){
            printf("receive droprate error %d\n",errno);
        }
        else{
            double droprate=pfmsg.droprate;
            time_t t2=time(NULL);
            if(t2!=t1){
                dropfile<<droprate<<endl;
                printf("droprate:%f\n",droprate);
                t1=t2;
            }
            if(t2-t0>=120){
                dropfile.close();
                dropfile.open("droprate.txt");
                t0=t2;
            }
        }
    }
    
}
void thread2(){
    while(1){
        pfmsg.mtype=2;
        if(msgrcv(id,(void *)&pfmsg,255,pfmsg.mtype,0) < 0){
            printf("msgrcv mtype=2 error\n");
        }
        else{
            printf("need to change bpf,now is :%s\n",pfmsg.nowbpffilter);
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
                        int ret=msgsnd(pfmsg.id,(void *)&pfmsg,255,0);
                        if(ret<0) printf("send back bpf msg error %d\n",errno);
                        else{
                            printf("next %s\n",bpfvec[i+1].c_str());
                            swap(bpfvec[i+1],bpfvec[0]);
                        }
                    } //发送回去下一个，调整bpfs.txt的文件排序，重写吧
                    else{
                        int ret=msgsnd(pfmsg.id,(void *)&pfmsg,255,0);
                        if(ret<0) printf("send back bpf msg error %d\n",errno);
                        else{
                            printf("next my self %s\n",bpfvec[i].c_str());
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
}
void thread3(){
    while(1){
        pfmsg.mtype=3;//向上调整，需要过滤强度低一些
        if(msgrcv(id,(void *)&pfmsg,255,pfmsg.mtype,0) < 0){
            printf("msgrcv mtype=3 error\n");
        }
        else{
            printf("need to change bpf,now is :%s\n",pfmsg.nowbpffilter);
            bpfs.open("bpfs.txt",ios::in);
            vector<string> bpfvec;
            string line;
            while(getline(bpfs,line)){
                bpfvec.push_back(line);
            }
            bpfs.close();
            sort(bpfvec.begin(),bpfvec.end(),[](string a, string b){return a.size() < b.size();});
            //按照长短排优先级，越短的过滤强度越低，越靠前，mtype=3，向上调，需要过滤强度低一些
            for(int i=0;i<bpfvec.size();i++){
                if(bpfvec[i]==pfmsg.nowbpffilter){
                    if(i-1>=0){
                        strcpy(pfmsg.nowbpffilter,bpfvec[i-1].c_str());
                        int ret=msgsnd(pfmsg.id,(void *)&pfmsg,255,0);
                        if(ret<0) printf("send back bpf msg error %d\n",errno);
                        else{
                            printf("next %s\n",bpfvec[i+1].c_str());
                            swap(bpfvec[i-1],bpfvec[0]);
                        }
                    } //发送回去上一个，调整bpfs.txt的文件排序，重写吧
                    else{
                        int ret=msgsnd(pfmsg.id,(void *)&pfmsg,255,0);
                        if(ret<0) printf("send back bpf msg error %d\n",errno);
                        else{
                            printf("next my self %s\n",bpfvec[i].c_str());
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
}
int main(int argc, char* argv[])
{  
	if(id == -1){
		printf("open msg error \n");
		return 0;
	}
    pfmsg.id=id;
    pfmsg.key=key;
    thread th1(thread1);  
    th1.detach(); 
    cout<<"th1"<<endl;
    thread th2(thread2);  
    th2.detach();
    cout<<"th2"<<endl;
    thread th3(thread3);  
    th3.detach();
    cout<<"th3"<<endl;
    while(1);
	return 0;
}