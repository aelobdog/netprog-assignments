// Solution to Network Programming, Lab Exercise 2
//    author : Ashwin Godbole
//        id : 2018B5A70423P
// -----------------------------------------------

// importing library functions.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// global variable to keep track of SIGUSR1 count.
int sigusr1_count = 0;

// increment global variable when handler is called.
void handle_sigusr1(int sig) {
   sigusr1_count ++;
}

// print the value of the global variable that tracks
// the number of messages exchanged between C1 and C2.
void handle_sigusr2(int sig) {
   sleep(1);
   printf("Number of messages exchanged between C1 and C2: %d\n", sigusr1_count);
   fflush(stdout);
   exit(0);
}

// function that prints error and causes the process
// to terminate.
void report_error(const char * error) {
   printf("ERROR: %s failed.\n", error);
   fflush(stdout);
   exit(1);
}

int main(int argc, char* argv[]) {
  // check number of arguments passed.
  if (argc < 3) {
    printf("USAGE: ./main <m> <n>\n");
    fflush(stdout);
    exit(1);
  }

  int id = 0; // to uniquely identify each project.
  
  int m, n; // to capture the command line arguments.
  m = atoi(argv[1]);
  n = atoi(argv[2]);
  
  int pipe_1[2], pipe_2[2]; // create two pipes.
  if (pipe(pipe_1) == -1) report_error("pipe creation (1)");
  if (pipe(pipe_2) == -1) report_error("pipe creation (2)");
  
  // create the first child process.
  int c1 = fork();
  if (c1 == -1) report_error("fork (1)"); 

  // assign id=1 to the first child.
  if (c1 == 0) {
    id = 1;
  } 
  else {
    // create the second child process.
    int c2 = fork();
    if (c2 == -1) report_error("fork (2)");

    // assign id=2 to the second child.
    if (c2 == 0) {
      id = 2;
    }
  }

  // child process C1's body.
  if (id == 1) {
    int child_one_rcv = m;

    close(pipe_1[1]); // close the write end of pipe_1.
    close(pipe_2[0]); // close the read end of pipe_2.
    
    while (child_one_rcv != 0) {
      // write the value of "m" or the value received into pipe_2.
      if (write(pipe_2[1], &child_one_rcv, sizeof(child_one_rcv)) == -1)
	report_error("child one's write");

      kill(getppid(), SIGUSR1); // send a SIGUSR1 signal to the parent.

      // read the next value that must be used as "m".
      if (read(pipe_1[0], &child_one_rcv, sizeof(child_one_rcv)) == -1)
	report_error("child one's read");
      
      printf("CHILD 1: read %d\n", child_one_rcv);
    }

    // if the received value is 0, send SIGUSR2 to the parent.
    kill(getppid(), SIGUSR2);
    // close the open ends of the pipes.
    close(pipe_1[0]);
    close(pipe_2[1]);
  }

  // child process C2's body.
  if (id == 2) {
    int child_two_rcv = -1;
    close(pipe_2[1]); // close the write end of pipe_2
    close(pipe_1[0]); // close the read end of pipe_1
    while (child_two_rcv != 0) {
      // read a value from pipe_2
      if (read(pipe_2[0], &child_two_rcv, sizeof(child_two_rcv)) == -1)
	report_error("child two's read");
      
      printf("CHILD 2: read %d\n", child_two_rcv);
      
      child_two_rcv = child_two_rcv / n; // dividing the received value by "n"
      
      // writing the value calculated to pipe_1
      if (write(pipe_1[1], &child_two_rcv, sizeof(child_two_rcv)) == -1)
	report_error("child two's write");

      kill(getppid(), SIGUSR1); // send a SIGUSR1 signal to the parent.
      
    }
    // if the received value is 0, send SIGUSR2 to the parent.
    kill(getppid(), SIGUSR2);
    // close the open ends of the pipes.
    close(pipe_1[1]);
    close(pipe_2[0]);
  }

  // parent process' body.
  if (id == 0) {
    // set up signal handlers for SIGUSR1 and SIGUSR2
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
    // wait for the child processes to terminate
    wait(NULL);
    wait(NULL);
  }
  
  return 0;
}
