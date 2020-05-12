#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <linux/limits.h>
#include <time.h>

#include "prensa.h"
#include "rwoper.h"

void signalHandler(int sig){
	if(sig == SIGUSR1){
		int ac = (random() % 100 < 50);
		char ans[2];
		if(ac) strcpy(ans, "Y");
		else strcpy(ans, "N");

		int fdToExec = open(JUD_EXEC_PIPE, O_WRONLY); 
		write(fdToExec, ans, 2);
		close(fdToExec);
	}
	if(sig == SIGUSR2){
		int ac = (random() % 100 < 50);
		char ans[2];
		if(ac) strcpy(ans, "Y");
		else strcpy(ans, "N");

		int fdToLeg = open(JUD_LEG_PIPE, O_WRONLY); 
		write(fdToLeg, ans, 2);
		close(fdToLeg);
	}
}

int execAction(int nLines, char action[MAX_ACTION][MAX_ACT_LINE], char *dir, pid_t idExec, pid_t idLeg, pid_t idJud){
	FILE *fp = NULL, *aux = NULL;
	int success = 1;
	char com[20], inst[MAX_ACT_LINE], fileName[PATH_MAX];

	for(int i = 1; i < nLines-2; i++){
		cutString(action[i],com,inst);
		if(strcmp(com, "exclusivo:") == 0){
			strncpy(fileName, dir, sizeof(fileName));
			strncat(fileName, inst, strlen(inst) - 1);
			openGovtFile(&fp, fileName, 1, 0);
		}
		else if(strcmp(com, "inclusivo:") == 0){
			strncpy(fileName, dir, sizeof(fileName));
			strncat(fileName, inst, strlen(inst) - 1);
			openGovtFile(&fp, fileName, 0, 0);
		}
		else if(strcmp(com, "leer:") == 0){
			if(!readFromFile(fp, inst)){
				success = 0;
				break;
			}
		}
		else if(strcmp(com, "anular:") == 0){
			if(readFromFile(fp, inst)){
				success = 0;
				break;
			}
		}
		else if(strcmp(com, "escribir:") == 0){
			if(writeToFile(fp, inst) < 0){
				success = 0;
				break;
			}
		}
		else if(strcmp(com, "aprobacion:") == 0){
			int p = 1;
			
			if(strcmp(inst, "Congreso\n") == 0)
				p = aprovalFrom(LEG_JUD_PIPE, idLeg, SIGUSR2);
			else if(strcmp(inst, "Tribunal Supremo\n") != 0)
				p = aprovalFrom(EXEC_JUD_PIPE, idExec, SIGUSR2);
			
			if(!p){
				success = 0;
				break;
			}
		}
		else if(strcmp(com, "reprobación:") == 0){
			int p = 0;
			
			if(strcmp(inst, "Congreso\n") == 0)
				p = aprovalFrom(LEG_JUD_PIPE, idLeg, SIGUSR2);
			else if(strcmp(inst, "Tribunal Supremo\n") != 0)
				p = aprovalFrom(EXEC_JUD_PIPE, idExec, SIGUSR2);
			
			if(p){
				success = 0;
				break;
			}
		}
	}

	openGovtFile(&fp, NULL, 0, 1);

	return success;
}

int main(int argc, char **argv){
	if(argc < 2){
		fprintf(stderr, "Too few arguments\n");
		return 0;
	}
	
	pid_t idExec, idLeg, idJud;
	char action[MAX_ACTION][MAX_ACT_LINE];
	char planPath[PATH_MAX];
	char dir[PATH_MAX];

	key_t key = ftok("GovtStats", 42);
	int shmid = shmget(key, 6*sizeof(int), 0666 | IPC_CREAT);
	int *stats = (int *) shmat(shmid, NULL, 0);

	// Path of directory where Govt. files exist
	if(argc > 1)
		strncpy(dir, argv[1], sizeof(dir));
	else
		dir[0] = '\0';

	// Path of Executive govt plan
	strncpy(planPath, dir, sizeof(planPath));
	strcat(planPath, "Judicial.acc");

	// Pipe to read the other processes ID's
	int pfd = open(JUD_PIPE_NAME, O_RDONLY);
	char pids[100];
	read(pfd, pids, sizeof(pids));
	close(pfd);
	sscanf(pids, "%d %d %d", &idExec, &idLeg, &idJud);

	// Semaphore to sync in with press
	sem_t *syncSem = sem_open(PRESS_SYNC_SEM, O_CREAT, 0666, 0);

	// Set signal handler for passing of days and inform press
	signal(SIGUSR1, signalHandler);
	signal(SIGUSR2, signalHandler);

	sem_post(syncSem);
	sem_close(syncSem);

	// Pipe to comunicate with press
	pfd = open(PRESS_NAME, O_WRONLY);
	sem_t *syncSem2 = sem_open(PRESS_SYNC_SEM2, O_CREAT, 0666, 0);

	// Initialize random generator
	srandom(time(NULL));

	while(1){
		stats[5]++;
		int nLines = readAction(planPath, action);
		char msg[MAX_ACT_LINE];
		if(nLines == 0){
			// Ninguna accion fue escogida
			sprintf(msg, JUD_IDDLE_MSG);
		}
		else if(nLines < 3){
			fprintf(stderr, "The Judicial Govt Plan has been corrupted\n");
			break;
		}
		else{
			int success = execAction(nLines, action, dir, idExec, idLeg, idJud);
			
			if(random() % 100 >= 66) 
				success = 0;
			
			if(success)
				strncpy(msg, action[nLines-2] + 7, sizeof(msg));	
			else
				strncpy(msg, action[nLines-1] + 9, sizeof(msg));
			
			stats[2] += success;

			//if(success)
				//eraseAction(planPath, "/tmp/LegislativoReplica", action[0]);
		}
		writeToPress(pfd, msg, strlen(msg) + 1, syncSem2);
	}

	// Close pipe and semaphore
	close(pfd);
	sem_close(syncSem2);

}