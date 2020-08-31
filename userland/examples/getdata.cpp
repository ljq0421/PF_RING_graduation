/*g++ getdata.cpp -o getdata -lmysqlclient -std=c++11 -L/usr/lib64/mysql*/
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
#include<algorithm>
#include<arpa/inet.h>
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
	double droprate;
    char nowbpffilter[255];
    char mtext[5120];
    int id;
    key_t key;
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
long long handletime(const string &timestr){
    //YYYY:M:D:HH:mm:ss.xxxxxxxxx
    int maohao=0,hour=0,min=0,sec=0;
    for(int i=0;i<timestr.size();i++){
        if(timestr[i]==':') maohao++;
        else if(timestr[i]=='.') break;
        else if(maohao==3){
            hour=(timestr[i]-'0')+hour*10;
        }else if(maohao==4){
            min=(timestr[i]-'0')+min*10;
        }else if(maohao==5){
            sec=(timestr[i]-'0')+sec*10;
        }
    }
    long long res=hour*3600+min*60+sec;
    return res;
}
long long IPtoINT(const string & strIP){
    //将整数IP转换为字符串表示的IP
    long long dwAddr = inet_addr(strIP.c_str());
    return dwAddr;
}
vector<vector<long long> > data;
//xshell sstime lltime ssrcip trytime len num len/num
vector<float> indexOUSHI={0.027202,0.17289998,0.13315726,0.11151741,0.32148709,0.04911495,0.07559104,0.10903027};
long long YUZHI=6000;
MYSQL *conn;
MYSQL_RES *res;
MYSQL_ROW row;
long long id=0;
long long disOUSHI(vector<long long> a,vector<long long> b){
    long long res=0;
    for(int i=1;i<a.size();i++){
        res+=(a[i]-b[i])*(a[i]-b[i])*indexOUSHI[i-1];
    }
    return sqrt(res);
}
void gettruedata(){
    char query3[1024];
    sprintf(query3,"select * from ssh1 where id > %d",id);
    mysql_query(conn,query3);
    res=mysql_store_result(conn);
    //id fid stimestamp src_ip dst_ip src_port dst_port len num etimestamp src_mac dst_mac
    while ((row = mysql_fetch_row(res))){
        //处理开始时间、结束时间、持续时间
        long long stime=handletime(string(row[2]));
        long long etime=handletime(string(row[9]));
        long long ltime=etime-stime;
        //处理源ip、目的ip
        long long srcip=IPtoINT(string(row[3]));
        //处理数据流长度、数据流包数量
        long long len=stoll(row[7]);
        long long num=stoll(row[8]);
        long long xshell=1;
        long long trytime=1;
        id=stoll(row[0]);
        data.push_back({id,xshell,stime,ltime,srcip,trytime,len,num,len/num});
    }
}

bool ifwrong(long long id,string stime,string etime,long long len,long long num,string srcip,string dstip){
    //新到来的flow信息，与ssh1中正常flow信息，对比，计算加权欧几里得距离，最大的若干个，是否超过阈值
    gettruedata();
    long long sstime=handletime(stime);
    long long eetime=handletime(etime);
    long long lltime=eetime-sstime;
    long long ssrcip=IPtoINT(srcip);
    long long xshell=1;
    long long trytime=1;
    int num_data=0;
    vector<long long> a={id,xshell,sstime,lltime,ssrcip,trytime,len,num,len/num};
    vector<long long> dis;
    for(int i=0;i<data.size();i++){
        long long tmp=disOUSHI(a,data[i]);
        cout<<data[i][0]<<" "<<tmp<<endl;
        if(tmp>=YUZHI){
            num_data++;
        }
        if(num_data/data.size()>0.1) return false;
    }
    return true;
}
int main(int argc, char* argv[]){
    int id = 0,i=0;
    struct mymesg pfmsg;
    pfmsg.mtype=4;
    key_t key = ftok("/tmp",2);
    id = msgget(key,0666|IPC_CREAT);
    if(id == -1){
        printf("open msg error \n");
        return 0;
    }
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
    //得到往常的正常数据
    gettruedata();
    unordered_map<string,struct packet > umap;
    while(1){
        time_t now = time(0);
        tm *ltm = localtime(&now);
        char dump_str[5120];
        string element,stimestamp,etimestamp,src_ip,dst_ip,len,flags,src_mac,dst_mac,src_port,dst_port,flow_id;
        struct packet pac;
        int num=0;
        memset(dump_str,0,1024);
        if(msgrcv(id,(void *)&pfmsg,5120,pfmsg.mtype,0) < 0){
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

        if(src_port!="22" && dst_port!="22") continue;
        
        int start=tmp.find("len=",nIndex);
        len=tmp.substr(start+4,tmp.find("]",start)-start-4);
        start=tmp.find("flags=",nIndex);
        flags=tmp.substr(start+6,tmp.find("]",start)-start-6);
        flags=getflag(flags);
        start=tmp.find("hash=",nIndex);
        flow_id=tmp.substr(start+5,tmp.find("]",start)-start-5);
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
            char query[5120];
            //判断异常与否
            string stime=umap[flow_id].stimestamp.c_str();
            string etime=umap[flow_id].etimestamp.c_str();
            long long len=umap[flow_id].len;
            long long num=umap[flow_id].num;
            string srcip=umap[flow_id].src_ip.c_str();
            string dstip=umap[flow_id].dst_ip.c_str();
            if(ifwrong(0,stime,stime,len,num,srcip,dstip)){//正常插入ssh1
                sprintf(query,"insert into ssh1(fid,stimestamp,src_ip,dst_ip,src_port,dst_port,len,num,etimestamp,src_mac,dst_mac) \
                    values ('%s','%s','%s','%s','%s','%s','%d','%d','%s','%s','%s')",
                    umap[flow_id].flow_id.c_str(),umap[flow_id].stimestamp.c_str(),umap[flow_id].src_ip.c_str(),
                    umap[flow_id].dst_ip.c_str(),umap[flow_id].src_port.c_str(),
                    umap[flow_id].dst_port.c_str(),umap[flow_id].len,umap[flow_id].num,
                    umap[flow_id].etimestamp.c_str(),umap[flow_id].src_mac.c_str(),umap[flow_id].dst_mac.c_str());
            }else{//异常插入ssh2
                sprintf(query,"insert into ssh2(fid,stimestamp,src_ip,dst_ip,src_port,dst_port,len,num,etimestamp,src_mac,dst_mac) \
                    values ('%s','%s','%s','%s','%s','%s','%d','%d','%s','%s','%s')",
                    umap[flow_id].flow_id.c_str(),umap[flow_id].stimestamp.c_str(),umap[flow_id].src_ip.c_str(),
                    umap[flow_id].dst_ip.c_str(),umap[flow_id].src_port.c_str(),
                    umap[flow_id].dst_port.c_str(),umap[flow_id].len,umap[flow_id].num,
                    umap[flow_id].etimestamp.c_str(),umap[flow_id].src_mac.c_str(),umap[flow_id].dst_mac.c_str());
            }
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