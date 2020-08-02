/*g++ getData.cpp -o getData -lmysqlclient -std=c++11 -L/usr/lib64/mysql*/
#include<iostream>
#include<vector>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unordered_map>
#include<sys/msg.h>
#include<sys/ipc.h>
#include<getopt.h>
#include<ctime>
#include<mysql/mysql.h>
#define TH_FIN_MULTIPLIER	0x01
#define TH_SYN_MULTIPLIER	0x02
#define TH_RST_MULTIPLIER	0x04
#define TH_PUSH_MULTIPLIER	0x08
#define TH_ACK_MULTIPLIER	0x10
#define TH_URG_MULTIPLIER	0x20
using namespace std;
extern char *optarg;
struct mymesg{
  long int mtype;
  char mtext[10240];
};
struct packet{
  string stimestamp;
  string etimestamp;
  string src_ip;
  string dst_ip;
  int len;
  string flags;
  string src_mac;
  string dst_mac;
  string src_port;
  string dst_port;
  string flow_id;
  int num;
};

string getflag(string flags){
  int f=stoi(flags);
  int urg=0,ack=0,push=0,rst=0,syn=0,fin=0;
  if(f>=0x20){ urg=1;f-=0x20;}
  if(f>=0x10){ ack=1;f-=0x10;}
  if(f>=0x08){ push=1;f-=0x08;}
  if(f>=0x04){ rst=1;f-=0x04;}
  if(f>=0x02){ syn=1;f-=0x02;}
  if(f>=0x01){ fin=1;f-=0x01;}
  return to_string(fin)+to_string(syn)+to_string(rst)+to_string(push)+to_string(ack)+to_string(urg);  
}

int main(int argc, char* argv[]){
  int id = 0,i=0;
  char c;
  struct mymesg pfmsg;
  int QUEUE;
  while((c = getopt(argc,argv,"Q:")) != '?') {
    if((c == 255) || (c == -1)) break;
    switch(c) {
      case 'Q':
	QUEUE=atoi(optarg);
	break;
    }
  }
  key_t key = ftok("/tmp",QUEUE);
  id = msgget(key,0666|IPC_CREAT);
  if(id == -1){
    printf("open msg error \n");
    return 0;
  }

  MYSQL *conn;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char* server="192.168.149.144";
  char* user="root";
  char* password="123";
  char* database="test1";
  conn=mysql_init(NULL);
  if(!mysql_real_connect(conn,server,user,password,database,3306,NULL,0)){
    printf("Error connecting to database:%s\n",mysql_error(conn));
  }else{
    printf("Connected...\n");
  }

  unordered_map<string,struct packet > umap;
  while(1){
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char dump_str[1024];
    string element,stimestamp,etimestamp,src_ip,dst_ip,len,flags,src_mac,dst_mac,src_port,dst_port,flow_id;
    struct packet pac;
    int num=0;
    memset(dump_str,0,1024);
    if(msgrcv(id,(void *)&pfmsg,10240,1,0) < 0){
      printf("receive msg error \n");
      return 0;
    }
    strcpy(dump_str,pfmsg.mtext);
    string tmp=dump_str;
    stimestamp=to_string(1900+ltm->tm_year)+":"+to_string(1 + ltm->tm_mon)+":"+to_string(ltm->tm_mday)+":"+tmp.substr(0,tmp.find(" "));
    int nIndex = -1;
    for(i=0;i<4;i++){
      nIndex++;
      nIndex = tmp.find('[',nIndex);
    }
    src_mac=tmp.substr(nIndex+1,tmp.find('-',nIndex)-nIndex-2);
    nIndex = tmp.find("-> ",nIndex);
    dst_mac=tmp.substr(nIndex+3,tmp.find(']',nIndex)-nIndex-3);
	
    for(i=4;i<6;i++){
      nIndex++;
      nIndex = tmp.find('[',nIndex);
    }
    src_ip=tmp.substr(nIndex+1,tmp.find(':',nIndex)-nIndex-1);
    src_port=tmp.substr(tmp.find(':',nIndex)+1,tmp.find(" -> ",nIndex)-tmp.find(':',nIndex));
    nIndex = tmp.find("-> ",nIndex);
    dst_ip=tmp.substr(nIndex+3,tmp.find(':',nIndex)-nIndex-3);
    dst_port=tmp.substr(tmp.find(':',nIndex)+1,tmp.find("]",nIndex)-tmp.find(':',nIndex)-1);
    int start=tmp.find("len=",nIndex);
    len=tmp.substr(start+4,tmp.find("]",start)-start-4);
    start=tmp.find("flags=",nIndex);
    flags=tmp.substr(start+6,tmp.find("]",start)-start-6);
    flags=getflag(flags);
    start=tmp.find("hash=",nIndex);
    flow_id=tmp.substr(start+5,tmp.find("]",start)-start-5);
    cout<<"tmp:"<<tmp<<endl
	<<"stimestamp:"<<stimestamp<<endl
	<<"src_ip:"<<src_ip<<endl
	<<"dst_ip:"<<dst_ip<<endl
	<<"len:"<<len<<endl
    	<<"src_mac:"<<src_mac<<endl
	<<"dst_mac:"<<dst_mac<<endl
	<<"src_port:"<<src_port<<endl
	<<"dst_port:"<<dst_port<<endl
	<<"flags:"<<flags<<endl
	<<"flow_id:"<<flow_id<<endl;
    if(umap.find(flow_id)==umap.end()){
      pac.stimestamp=stimestamp;
      pac.src_ip=src_ip;
      pac.dst_ip=dst_ip;
      pac.len=stoi(len);
      pac.src_mac=src_mac;
      pac.dst_mac=dst_mac;
      pac.src_port=src_port;
      pac.dst_port=dst_port;
      pac.flags=flags;
      pac.flow_id=flow_id;
      pac.num=1;
      umap[flow_id]=pac;
    }else{
      umap[flow_id].num++;
      umap[flow_id].len+=stoi(len);
    }
    if(flags[0]=='1' && umap[flow_id].etimestamp==""){//FIN,update etimestamp
      umap[flow_id].etimestamp=stimestamp;
      char query[10240];
      sprintf(query,"insert into ssh1(fid,stimestamp,src_ip,dst_ip,src_port,dst_port,len,num,etimestamp,src_mac,dst_mac) \
	values ('%s','%s','%s','%s','%s','%s','%d','%d','%s','%s','%s')",
	umap[flow_id].flow_id.c_str(),umap[flow_id].stimestamp.c_str(),umap[flow_id].src_ip.c_str(),
	umap[flow_id].dst_ip.c_str(),umap[flow_id].src_port.c_str(),
	umap[flow_id].dst_port.c_str(),umap[flow_id].len,umap[flow_id].num,
	umap[flow_id].etimestamp.c_str(),umap[flow_id].src_mac.c_str(),umap[flow_id].dst_mac.c_str());
      int t2=mysql_query(conn,query); 
      if(t2){
        printf("Error making query:%s\n",mysql_error(conn));
      }
      cout<<"query:"<<query<<endl;
    }

  }
  unordered_map<string,struct packet > ::iterator it;
  for(it=umap.begin();it!=umap.end();it++){
    cout<<it->first<<" "<<it->second.stimestamp<<endl;
  }

  mysql_close(conn);
  
  return 0;

}
