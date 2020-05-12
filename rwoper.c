#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <linux/limits.h>
#include <sys/file.h>
#include <time.h>

#include "prensa.h"
#include "rwoper.h"

int writeToPress(int fd, char *msg, int nBytes, sem_t *syncSem){
	if(nBytes >= PIPE_BUF){
		fprintf(stderr, "Headline is too long!\n");
		return 0;
	}

	flock(fd, LOCK_EX);
	int wBytes = write(fd, msg, nBytes);
	sem_wait(syncSem);
	flock(fd, LOCK_UN);

	return wBytes;
}

int readAction(const char *filePath, char action[MAX_ACTION][MAX_ACT_LINE]){
	FILE *file = fopen(filePath, "r");

	if(file == NULL){
		fprintf(stderr, "Couldn't open file: %s\n", filePath);
		return 0;
	}

	int n = 0;
	int fd = fileno(file);
	char *line = malloc(MAX_ACT_LINE*sizeof(char));
	size_t len = MAX_ACT_LINE;

	flock(fd, LOCK_SH);
	while(getline(&line, &len, file) != -1){
		int r = random() % 100;
		if(r < 10){
			strncpy(action[n++], line, sizeof(action[0]) - 1);
			while(getline(&line, &len, file) != -1){
				if(strcmp(line, "\n") == 0) break;
				strncpy(action[n++], line, sizeof(action[0]) - 1);
			}
		}
		else{
			while(getline(&line, &len, file) != -1)
				if(strcmp(line, "\n") == 0) break;
		}
		if(n) break;
	}
	flock(fd, LOCK_UN);
	free(line);
	fclose(file);

	return n;
}

int writeToFile(FILE *f, char *s){
	int i = fprintf(f, s);
	return i;
}

int readFromFile(FILE *f, char *s){
	int n = 0;
	char *line = NULL;
	size_t len = 0;

	rewind(f);

	while(getline(&line, &len, f) != -1){
		if(strcmp(line, s) == 0){
			return 1;
		}
	}
	return 0;
}

void cutString(const char *s, char *head, char *tail){
	int i, n = strlen(s);
	for(i = 0; i < n; i++){ 		
		if(s[i] == ' ')
			break;
		if(s[i] == '\n'){
			head[i] = '\0';
			strcpy(tail, "\n");
			return;
		}
		head[i] = s[i];
	}
	head[i] = '\0';
	strcpy(tail, s + i + 1);
}

void openGovtFile(FILE **file, const char *path, int mode, int closeOnly){
	if(*file != NULL){
		int fd = fileno(*file);
		flock(fd, LOCK_UN);
		fclose(*file);
		*file = NULL;
	}
	if(!closeOnly){
		*file = fopen(path, "a+");
		int fd = fileno(*file);
		if(mode)
			flock(fd, LOCK_EX);
		else
			flock(fd, LOCK_SH);
	}
}

int aprovalFrom(const char *pipeName, pid_t aproverID, int sig){
	char ans[2];
	kill(aproverID, sig);
	
	int pfd = open(pipeName, O_RDONLY);
	read(pfd, ans, 2);
	close(pfd);
	
	return (ans[0] == 'Y');
}

void eraseAction(const char *filePath, char *replicaName, char *actionName){
	FILE *file = fopen(filePath, "r");
	FILE *replica = fopen(replicaName, "w+");

	if(file == NULL){
		fprintf(stderr, "Couldn't open file: %s\n", filePath);
		return;
	}

	int fd = fileno(file);
	char *line = malloc(MAX_ACT_LINE*sizeof(char));
	size_t len = MAX_ACT_LINE;

	flock(fd, LOCK_SH);

	while(getline(&line, &len, file) != -1){
		if(strcmp(actionName, line) != 0){
			fprintf(replica, "%s", line);
			while(getline(&line, &len, file) != -1){
				fprintf(replica, "%s", line);
				if(strcmp(line, "\n") == 0) break;
			}
		}
		else{
			while(getline(&line, &len, file) != -1)
				if(strcmp(line, "\n") == 0) break;
		}
	}

	rewind(replica);

	flock(fd, LOCK_EX);

	FILE *file2 = fopen(filePath, "w");

	while(getline(&line, &len, replica) != -1)
		fprintf(file2, "%s", line);

	flock(fd, LOCK_UN);

	fclose(file);
	fclose(file2);
	fclose(replica);
}