#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include "pidlist.h"

Pid_List create_pid_list(){
	return NULL;
}

void add_pid_list(Pid_List *l, pid_t pid, char* command){
	Pid_List el = malloc(sizeof(*el));
	el->next = *l;
	el->pid = pid;
	el->command = malloc((strlen(command)+1) * sizeof(char));
	strcpy(el->command, command);
	*l = el;
}

void clean_pid_list(Pid_List *l){
	Pid_List it;
	Pid_List_Element tl;
	tl.next = *l;
	it = &tl;
	while(it->next != NULL){
		int status;
		waitpid(it->next->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
		//printf("pid: %d, command: %s, exited: %d, status: %d\n", it->next->pid, it->next->command, WIFEXITED(status)||WIFSIGNALED(status), status);
		if(WIFEXITED(status) /*l|| WIFSIGNALED(status)*/){
		printf("pid: %d, command: %s, exited: %d, status: %d\n", it->next->pid, it->next->command, WIFEXITED(status)||WIFSIGNALED(status), status);
				free(it->next->command);
				Pid_List tmp = it->next;
				it->next = it->next->next;
				free(tmp);
		}
		else{
			it = it->next;
		}
	}
	*l = tl.next;
}

void print_pid_list(Pid_List l){
	Pid_List it = l;
	while(it != NULL){
		printf("%d\t %s\n", it->pid, it->command);
		it = it->next;
	}
}
