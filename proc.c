#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

const char * getAction(char *dir, int prob){
	FILE *fp
	fp= fopen(*dir, r);
	
}

int main(int argc, char **argv){
	if(argc<2) return 0;
	int days_len, day;
	sscanf(argv[1], "%d", &days_len);
	char dir[100];
	dir="";
	if(argc>2){
		strcpy(dir, argv[2]);
	}
	day=0;
	int id_exec = fork();
	if(id_exec==0){
		//Ejecutivo
		char dir_ex[120];
		strcpy(dir_ex, dir);
		strcpy(dir_ex, "/Ejecutivo.acc");
		char action[500];
		while(day<days_len){
			action= readacc(dir_ex);
		}
	}
	else{
		int id_leg = fork();
		if(id_leg==0){
			//Legislativo
			char dir_leg[120];
			strcpy(dir_leg, dir);
			strcpy(dir_leg, "/Legislativo.acc");
			char action[500];
			while(day<days_len){
				action= readacc(dir_leg);	
			}
		}
		else{
			int id_jud = fork();
			if(id_jud==0){
				//Judicial
				char dir_jud[120];
				strcpy(dir_jud, dir);
				strcpy(dir_jud, "/Judicial.acc");
				char action[500];
				while(day<days_len){
					action= readacc(dir_jud);
				}
			}
			else{
				//Press
				while(day<days_len){
					
				}
			}			
		}
	}
}