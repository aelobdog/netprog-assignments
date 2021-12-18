// Solution to Network Programming, Lab Exercise 3
//    author : Ashwin Godbole
//        id : 2018B5A70423P
// -----------------------------------------------

// For this exercise, I have interpretted "paragraph"
// of input as a multiline input. For this reason,

//     scanf("%1023[^~]", msg.text);

// has been used. This makes sure to read atmost 1023
// characters (max size of input buffer has been set
// to 1024) and also that input stops when the '~'
// character is encountered in the input. Alternatively
// the user may also terminate the input reading process
// using ^d (ctrl-d).

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

struct message {
  long type;
  char text[1024];
};

int proc_id;
struct message msg;

int main() {
  key_t key;
  int msg_q_id, iter, cid;

  // set parent process' proc_id to 0
  proc_id = 0;
  
  // generate a unique key for the message queue
  key = ftok("msq.c", 15);
  
  // create a message queue using the key obtained
  // permissions set to 0666 => read and write (no exec)
  // IPC_CREAT to create the queue
  msg_q_id = msgget(key, 0666 | IPC_CREAT);
  
  if (msg_q_id < 0) {
    perror("msgget");
    exit(1);
  }
  
  // creating 5 child processes
  for (iter = 0; iter < 5; iter++) {
    switch ((cid = fork())) {
    case -1:
      perror("fork");
      exit(1);
    case 0:
      // child process
      proc_id = iter + 1;
      break;
    }

    // stop child processes from creating their own
    // child processes
    if (cid == 0) break;
  }

  // switch case to tell processes to do their jobs
  switch (proc_id) {
    
  case 0:
    // parent process
    // message type for parent set to 1
    msg.type = 1;
    // reading a paragraph from stdin using scanf
    // since limit of message text is 1024, 1023
    // is the max size given to scanf
    scanf("%1023[^~]s", msg.text);
    // printing out the message read
    printf("\nparent [pid : %d] (initial input)\n", getpid());
    fputs(msg.text, stdout);
    printf("\n");
    fflush(stdout);
    // send the message as is to the first child
    msgsnd(msg_q_id, &msg, sizeof(msg), 0);
    // after the last child process is done with
    // its operations, it sends the parent the
    // final modified message
    msgrcv(msg_q_id, &msg, sizeof(msg), 6, 0);
    printf("\nparent [pid : %d] (final output)\n", getpid());
    fputs(msg.text, stdout);
    printf("\n");
    break;

  case 1:
    // child process 1
    // objectives:
    //   - convert all lowercase characters to uppercase
    
    // receive message from queue
    msgrcv(msg_q_id, &msg, sizeof(msg), proc_id, 0);
    printf("\nchild 1 [pid : %d]\n", getpid());

    // convert all lowercase letters to uppercase
    for (iter = 0; msg.text[iter] != '\0'; iter++) {
      int ch = msg.text[iter];
      if(islower(ch))
	msg.text[iter] = toupper(ch);
    }
    // print the modified message to stdout
    fputs(msg.text, stdout);
    printf("\n");
    // set the message type to 2 before sending the message
    msg.type = proc_id + 1;
    // send the modified message to the second child
    msgsnd(msg_q_id, &msg, sizeof(msg), 0);
    break;

  case 2: case 4:
    // child process 2 and 4
    // objectives:
    //   - remove the first character of the message
    
    // receive message from queue
    msgrcv(msg_q_id, &msg, sizeof(msg), proc_id, 0);
    printf("\nchild %d [pid : %d]\n", proc_id, getpid());

    // remove the first character of the string
    for (iter = 0; msg.text[iter+1] != '\0'; iter++)
      msg.text[iter] = msg.text[iter+1];
    msg.text[iter] = '\0';
    
    // print the modified message to stdout
    fputs(msg.text, stdout);
    // set the message type to (proc_id + 1) before it
    msg.type = proc_id + 1;
    printf("\n");
    // send the modified message to the next child
    msgsnd(msg_q_id, &msg, sizeof(msg), 0);
    break;

  case 3: case 5:
    // child process 3 and 5
    // objectives:
    //   - remove the last character of the message
    
    // receive message from queue
    msgrcv(msg_q_id, &msg, sizeof(msg), proc_id, 0);
    printf("\nchild %d [pid : %d]\n", proc_id, getpid());

    int msg_length = strlen(msg.text);
    // get the last non special character from the string
    // this is required because the message is originally
    // obtained from stdin may contain a trailing newline.
    // also, the null terminated string should remain null
    // terminated at the end of this operation, the '\0'
    // must be preserved.
    for (iter = msg_length-1; iter >= 0; iter--) {
      if (msg.text[iter] != '\n' && msg.text[iter] != '\0')
	break;
    }

    // remove the last character of the string
    for (; msg.text[iter+1] != '\0'; iter++)
      msg.text[iter] = msg.text[iter+1];
    msg.text[iter] = '\0';

    // print the modified message to stdout
    fputs(msg.text, stdout);
    printf("\n");
    // set the message type to (proc_id + 1) before it
    msg.type = proc_id + 1;
    // send the modified message to the next child
    msgsnd(msg_q_id, &msg, sizeof(msg), 0);
    break;
  }
}
