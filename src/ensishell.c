/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <wordexp.h>
#include <errno.h>
#include <signal.h>

#include "variante.h"
#include "readcmd.h"
#include "pidlist.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>


Pid_List pidTable;

void terminationHandler(int signum, siginfo_t *info, void *context){
	Pid_List process = find_pid_list(pidTable, info->si_pid);
	//printf("info: %p, pid: %d, uid: %d\n", info, info->si_pid, info->si_uid);
	if(process == NULL){
		printf("Process %d terminated\n", info->si_pid);
	}
	else{
		if(!process->background){
			return;
		}
		struct timeval time;
		gettimeofday(&time, NULL);
		printf("Process %d:%s terminated after %ldms\n", process->pid, process->command, (-process->time.tv_sec + time.tv_sec)*1000+(-process->time.tv_usec+time.tv_usec)/1000);
	}
}

pid_t create_processes(char ***processes, char *in, char *out, unsigned char background){
	int pipe_in[2]; 
	int pipe_out[2];
	pid_t last_process;
	for(int i = 0; processes[i] != NULL; i++){
		if(i >= 2){ 
			close(pipe_in[0]);
			close(pipe_in[1]); 
		}
		pipe_in[0] = pipe_out[0];
	   	pipe_in[1] = pipe_out[1];
		if(processes[i+1] != NULL){
			pipe(pipe_out);
		}

//		wordexp_t p;
//		wordexp(processes[i], &p, 0);
//		for(int j = 0; j < p.we_wordc; j++){
//			printf("%s\n", p.we_wordv[j]);
//		}
//		wordfree(&p);

		pid_t process = fork();
		if(process == 0){
			if(i == 0 && in != NULL){
				int inDescriptor = open(in, O_RDONLY);
				dup2(inDescriptor, 0);
				close(inDescriptor);
			}
			if(i!=0){
				dup2(pipe_in[0], 0);
				close(pipe_in[0]);
				close(pipe_in[1]);
			}
			if(processes[i+1] != NULL){
				dup2(pipe_out[1], 1);
				close(pipe_out[0]);
				close(pipe_out[1]);
			}
			else if(out != NULL){
				int outDescriptor = open(out, O_WRONLY | O_CREAT);
				dup2(outDescriptor, 1);
				close(outDescriptor);
			}
			int err = execvp(processes[i][0], processes[i]);
			if(err == -1){
				perror(NULL);
				exit(0);
			}
		}
		else{
			add_pid_list(&pidTable, process, processes[i][0], background);
			if(processes[i+1]==NULL){
				last_process = process;
			}
		}
		if(processes[i+1] == NULL && i > 0){
			close(pipe_in[0]);
			close(pipe_in[1]);
		}
	}
	return last_process;
}

int executer(char *line)
{
	/* Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
	//printf("Not implemented: can not execute %s\n", line);

	
	struct cmdline *l;
	l = parsecmd( & line);
	if (l->err) {
		/* Syntax error, read another command */
		printf("error: %s\n", l->err);
		return 1;
	}
	pid_t process = create_processes(l->seq, l->in, l->out, l->bg);
	if(!l->bg){
		while(1){
			waitpid(process, NULL, 0);
			if(waitpid(process, NULL, 0) == -1){
				break;
			}
		}
		//waitpid(process, NULL, 0);
	}

	/* Remove this line when using parsecmd as it will free it */
	//free(line);
	
	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#ifdef USE_GNU_READLINE
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}


int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#ifdef USE_GUILE
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

		pidTable = create_pid_list();

		struct sigaction act;
		memset(&act, '\0', sizeof(act));
		act.sa_sigaction = &terminationHandler;
		act.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
		if(sigaction(SIGCHLD, &act, NULL) == -1){
			perror("sigaction");
		}

	while (1) {
		char *line=0;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
		}
		else if(!strncmp(line, "jobs", 4)){
			clean_pid_list(&pidTable);
			print_pid_list(pidTable);
#ifdef USE_GNU_READLINE
		add_history(line);
#endif
			continue;
		}

#ifdef USE_GNU_READLINE
		add_history(line);
#endif


#ifdef USE_GUILE
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif

		executer(line);
//		struct cmdline *l;
//		int i, j;
//		/* parsecmd free line and set it up to 0 */
//		l = parsecmd( & line);
//
//		/* If input stream closed, normal termination */
//		if (!l) {
//		  
//			terminate(0);
//		}
//		
//
//		
//		if (l->err) {
//			/* Syntax error, read another command */
//			printf("error: %s\n", l->err);
//			continue;
//		}
//
//		if (l->in) printf("in: %s\n", l->in);
//		if (l->out) printf("out: %s\n", l->out);
//		if (l->bg) printf("background (&)\n");
//
//		/* Display each command of the pipe */
//		for (i=0; l->seq[i]!=0; i++) {
//			char **cmd = l->seq[i];
//			printf("seq[%d]: ", i);
//                        for (j=0; cmd[j]!=0; j++) {
//                                printf("'%s' ", cmd[j]);
//                        }
//			printf("\n");
//		}
	}

}
