#ifndef _PIDLIST_H_
#define _PIDLIST_H_

#include <unistd.h>

typedef struct list_s{
	struct list_s *next;
	pid_t pid;
	char *command;
} Pid_List_Element;

typedef Pid_List_Element *Pid_List;

Pid_List create_pid_list();
void add_pid_list(Pid_List *l, pid_t pid, char* command);
void clean_pid_list(Pid_List *l);
void print_pid_list(Pid_List l);

#endif
