#include<iostream>
#include<stdlib.h>
using namespace std;
int main(){
	while(1){
		for(int i=50;i<70;i+=2){
			printf("%d\n",i);
			string str="./testssh.sh "+stoi(i));
		}
	}
	return 0;
}
			
