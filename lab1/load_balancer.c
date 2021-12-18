// Solution to Network Programming, Lab Exercise 1
//    author : Ashwin Godbole
//        id : 2018B5A70423P
// -----------------------------------------------
// In this solution, I have used realtime signals
// instead of regular signals because :
//
//    1) sigqueue can send signals that get queued
//       instead of kill, who's signals don't get 
//       queued.
//
//    2) siginfo_t has a field si_value which can 
//       be used to send data alongside a signal
//       being sent using sigqueue.
// -----------------------------------------------

// including all the library headers required
#include <bits/types/siginfo_t.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// declaring global variables
int n, line_number, total_lines;
FILE* file;
char *filename;
int *child;

void usage(void) {
   printf("USAGE : ./load_balancer.c <filename> <N>\n");
   exit(1);
}

// Get the total number of lines present in a file.
// Since this function opens and closes its own 
// FILE pointer, the file offset change caused by
// this function is not refected in the calling 
// fucntion.  
int get_lines_in_file(char* filen) {
   FILE* temp = fopen(filen, "r");
   char buf[512];
   char *res = fgets(buf, sizeof(buf), temp);
   int lines = 0;
   while(res != NULL) {
      res = fgets(buf, sizeof(buf), temp);
      lines ++;
   }
   fclose(temp);
   return lines;
}

// given the number of lines to skip, read and reject
// those many lines. Since the reading is done from a
// FILE pointer passed as the argument, the file offset 
// that is changed by this function is also reflected
// in the calling function.
void skip_n_lines_in_file(int lines_to_skip, FILE* file) {
   char buf[512];
   for (int l=0; l<lines_to_skip; l++) {
      fgets(buf, sizeof(buf), file);
   }
}

// A signal handler assigned to signal SIGUSR1 in the
// parent process. This signal handler, when invoked,
// kills all child processes in the reverse order of
// their creation, frees the heap memory allocated for
// the child[] array, and proceeds to kill the parent
// process. After this function is invoked, all 
// processes created by the parent process, including
// itself are killed.
void release_memory(int sig) {
   sleep(1);
   printf("\nFile reading complete.\n");
   printf("Killing child processes in reverse order.\n");
   printf("Killing parent process.\n");
   fflush(stdout);
   // killing all processes in reverse order of their creation.
   for (int i=n-1; i>=0; i--) {
      kill(child[i], SIGKILL);
   }
   free(child);  // freeing child[] heap memory.
   child = NULL; // just for safety, setting child to NULL after 
                 // freeing it.
   exit(0);      // quit the parent process.
}

// A signal handler assigned to the realtime signal 'SIGRTMIN+1'
// in the child processes. This signal handler, when invoked,
// causes the child process to:
//    1. open the file that is given in the cmd line arguments 2. skip the correct number of lines based on the number sent in the si_value field by the parent process 3. read a line of text from the file 4. sleep for 1 second 5. print the line read (if it was read correctly
// Optionally, if the line it just read was the last line in the
// file, it sends a SIGUSR1 signal to its parent signifying that
// the whole file has been read and all the processes have completed
// the assigned task and are ready to be killed.
void handle_rtsig(int sig, siginfo_t* info, void *unused) {
   file = fopen(filename, "r");
   skip_n_lines_in_file(info->si_value.sival_int, file);

   char buf[512];
   char *res = fgets(buf, sizeof(buf), file);
   sleep(1);
   if (res != NULL){
      buf[strlen(buf) - 1] = '\0'; // This is because reading a single line
                                   // includes the '\n' at the end. So this
                                   // replaces that '\n' with a '\0' for 
                                   // better printing in the next line.
      printf("[%d] read : %s\n", getpid(), buf);
      fflush(stdout);
   }
   fclose(file);

   // Signal to the parent that the last line has been read.
   if (info->si_value.sival_int == get_lines_in_file(filename) - 1) {
      kill(getppid(), SIGUSR1);
   }
}

// main() : the entry point of the program.
int main (int argc, char* argv[]) {
   if (argc < 3) {
      printf("2 arguments expected. Got %d\n", argc-1);
      usage();
   }

   // Initializing some global variables.
   filename = argv[1];
   n = atoi(argv[2]);
   line_number = get_lines_in_file(filename);
   child = (int*)malloc(sizeof(int)*n);

   // Register the signal handler for SIGUSR1
   // Using signal() for this because the signal handler
   // is a simple signal handler and does not require the
   // use of the flexibility of sigaction.
   signal(SIGUSR1, release_memory); 

   // Register the signal handler for SIGRTMIN+1
   // Using sigaction for this because the signal handler
   // for this signal uses the siginfo_t struct.
   struct sigaction act = {0};
   act.sa_sigaction = handle_rtsig;
   act.sa_flags = SA_SIGINFO;
   sigaction(SIGRTMIN+1, &act, NULL);

   int cid; // local variable to store the PIDs of the child proecsses
   
   printf("\nLOG: Creating child processes.\n");
   // This loop creates 'n' child processes.
   for(int i=0; i<n; i++) {
      switch ((cid = fork())) {
         case -1:
            // ERROR : fork() didn't work as expected.
            printf("ERROR: fork() failed.\n");
            exit(1);
         case 0:
            // Child process
            // pause() makes the child process suspend its
            // execution until it recieves a signal.
            pause();
         default:
            // Parent process
            // Makes a note of the PID of the child process
            // that was just created in the child[] array
            child[i] = cid;
      }
      if (cid == 0) break; // this prevents the child process from
                           // executing this loop in its body.
                           // Otherwise the child processes would 
                           // make more child processes (which is not
                           // what is required for this program.
   }

   // If the process is a child process, this loop puts it in an 
   // infinite loop of "pausing". This ensures that the child process
   // which is paused,
   //    1. recieves a signal from the parent
   //    2. executes the signal handler
   //    3. pauses again and waits for the next signal
   if (cid == 0) while (1) pause();
   
   // If the process is the parent process, this loop assigns each
   // child process a line number and sends it a signal to read the
   // required line from the file. sigqueue() is a mechanism to 
   // realtime signals to the required process, while queue-ing the 
   // signals, and passing a value alongside the signal.
   //
   // The child processes, when free, are assigned more work in a 
   // ROUND ROBIN fashion, till all the lines are read.
   if (cid > 0) {

      printf("LOG: Child processes created : \n");
      for (int i=0; i<n; i++) {
         printf("    child pid : %d\n", child[i]);
      }
      printf("\nLOG: Assigning reads:\n");
      fflush(stdout);

      for (int i=0; i<=line_number; i++) {
         union sigval value;
         value.sival_int = i;
         sigqueue(child[i%n], SIGRTMIN+1, value);
      }

      // The parent process waits till all the lines in the file
      // are read.
      pause();
      fflush(stdout);
   }
}
