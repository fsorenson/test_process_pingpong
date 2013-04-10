#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__



void print_trace(int nSig)
{
  printf("print_trace: got signal %d\n", nSig);

  void           *array[32];    /* Array to store backtrace symbols */
  size_t          size;     /* To store the exact no of values stored */
  char          **strings;    /* To store functions from the backtrace list in ARRAY */
  size_t          nCnt;

  size = backtrace(array, 32);

  strings = backtrace_symbols(array, size);

  /* prints each string of function names of trace*/
  for (nCnt = 0; nCnt < size; nCnt++)
    fprintf(stderr, "%s\n", strings[nCnt]);


  exit(-1);
}



void backtrace_on_(int signal) {
	struct sigaction action = {};
	action.sa_handler = DumpBacktrace;
	if (sigaction(signal, &action, NULL) < 0) {
		perror("sigaction()");
	}
}

void DumpBacktrace(int) {
	pid_t dying_pid = getpid();
	pid_t child_pid = fork();
	if (child_pid < 0) {
		perror("fork() while collecting backtrace:");
	} else if (child_pid == 0) {
		char buf[1024];
		sprintf(buf, "gdb -p %d -batch -ex bt 2>/dev/null | "
		"sed '0,/<signal handler/d'", dying_pid);
		const char* argv[] = {"sh", "-c", buf, NULL};
		execve("/bin/sh", (char**)argv, NULL);
		_exit(1);
	} else {
		waitpid(child_pid, NULL, 0);
	}
	_exit(1);
}


#endif
