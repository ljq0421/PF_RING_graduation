/* g++ getnewflow.cpp -o getnewflow -lmysqlclient -L/usr/lib64/mysql -std=c++11*/
#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>
#include<map>
#include<string>
#include <arpa/inet.h>
#include<unistd.h>

using namespace std;
int handletime(const string &timestr){
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
    int res=hour*3600+min*60+sec;
    return res;
}

long long IPtoINT(const string & strIP){
    //将整数IP转换为字符串表示的IP
    long long dwAddr = inet_addr(strIP.c_str());
    return dwAddr;
}

int main(int argc, char* argv[])
{
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
    mysql_query(conn,"select * from ssh1");
    res=mysql_store_result(conn);
    int id=0;
    //id fid stimestamp src_ip dst_ip src_port dst_port len num etimestamp src_mac dst_mac
    while ((row = mysql_fetch_row(res))){
        //处理开始时间、结束时间、持续时间
        long long stime=handletime(string(row[2]));
        long long etime=handletime(string(row[9]));
        long long ltime=etime-stime;
        //处理源ip、目的ip
        long long srcip=IPtoINT(string(row[3]));
        long long dstip=IPtoINT(string(row[4]));
        //处理数据流长度、数据流包数量
        long long len=stoi(row[7]);
        long long num=stoi(row[8]);
        id=stoi(row[0]);
    }
    cout<<id<<endl;
    while(1){
        char query3[1024];
        sprintf(query3,"select * from ssh1 where id > %d",id);
        mysql_query(conn,query3);
        res=mysql_store_result(conn);
        while ((row = mysql_fetch_row(res))){
            cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<endl;
            id=stoi(row[0]);
        }

        sleep(1);
    }
    return 0;
}
