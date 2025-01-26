/*
Jake Marlow
jshell.c
cs360
April 22, 2024
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "fields.h"
#include "dllist.h"
#include "jrb.h"

// what currently seemingly works
// executing one/multiple command(s)
// redirecting stdin with dup2
// redirecting stdout with dup2
// append to stdout with dub2
// need to write piping for any multiple command
// need to implement nowait

// gcc -I/home/jplank/cs360/include -o jshell jshell.c /home/jplank/cs360/lib/libfdr.a

typedef struct
{
   char *stdin;       /* Filename from which to redirect stdin.  NULL if empty.*/
   char *stdout;      /* Filename to which to redirect stdout.  NULL if empty.*/
   int append_stdout; /* Boolean for appending.*/
   int wait;          /* Boolean for whether I should wait.*/
   int n_commands;    /* The number of commands that I have to execute*/
   int *argcs;        /* argcs[i] is argc for the i-th command*/
   char ***argvs;     /* argcv[i] is the argv array for the i-th command*/
} Command;


void setCommand(Command *command) {
   command->argcs = malloc(500);
   command->n_commands = 0;
   command->wait = 1;
   command->append_stdout = 0;
   command->stdin = NULL;
   command->stdout = NULL;
   command->argvs = malloc(4000 * sizeof(char **)); // arbitrary size (should be big enough to handle any inputs)
}

void freeCommand(Command *command) {
   int i, j;
   
   for(i = 0; i < command->n_commands; i++) { //free argv
      for(j = 0; j < command->argcs[i]; j++) {
         free(command->argvs[i][j]);
      }
      free(command->argvs[i]);
   }

      free(command->argvs);
      free(command->argcs);
   

   free(command->stdin);
   free(command->stdout);
   free(command);
}

void setInOut(int i, Command *command, int (*pipeArray)[2])
{
   int fd0, fd1; // file descriptors

   if (i == 0)
   { // set up first command to read from stdin or wherever specified by user
      if (command->stdin != NULL)
      {
        
         fd0 = open(command->stdin, O_RDONLY); // open file
         if (fd0 < 0)
         {
            perror("failed opening input file 1");
            exit(1);
         }

         if (dup2(fd0, 0) < 0) // reads from where user says to
         {                     // call dup2 on file with 0 to redirect stdin
            perror("failed dup for input file 2");
            exit(1);
         }
         close(fd0);
      }
      else
      {
         
      }

    
      close(pipeArray[i][0]);
      close(1);
      if (dup2(pipeArray[i][1], 1) != 1) // sends input into the pipe
      {                                  // call dup2 on file with 0 to redirect stdout
         perror("failed dup for output file 3");
         exit(0);
      }
   close(pipeArray[i][1]);
   }
   else if (i == command->n_commands - 1)
   {

      if (command->stdout != NULL)
      { // Redirect stdout
         if (command->append_stdout == 0)
         {
          
            fd1 = open(command->stdout, O_WRONLY | O_TRUNC | O_CREAT, 0644); // Open file
         }
         else
         {
           
            fd1 = open(command->stdout, O_WRONLY | O_APPEND | O_CREAT, 0644); // Open file in append mode
         }
         if (fd1 < 0)
         {
            perror("failed opening stdout");
            exit(2);
         }
         if (dup2(fd1, 1) < 0)
         { // Redirect stdout to file
            perror("failed dup for output file 4");
            exit(1);
         }
         close(fd1);
      }
      else
      {
        
      }

     
      close(pipeArray[i - 1][1]);

     close(0);

      if (dup2(pipeArray[i - 1][0], 0) != 0)
      { // Redirect stdin to pipe
         perror("failed dup for output file 5");
         exit(1);
      }
      close(pipeArray[i - 1][0]);
   }
   else
   { // in the middle
    
      close(pipeArray[i][0]);
      close(1);
      if (dup2(pipeArray[i][1], 1) != 1)
      { // setting stdout to pipe
         perror("failed dup for output file 6");
         exit(1);
      }
      close(pipeArray[i][1]);

   
      close(pipeArray[i - 1][1]);
      close(0);
      if (dup2(pipeArray[i - 1][0], 0) != 0)
      { // setting stdin to pipe
         perror("failed dup for input file 7");
         exit(1);
      }
      close(pipeArray[i - 1][0]);
   }
}

void execute(Command *command)
{
   int i, pid, k, l, status, found;
   JRB children, tmp;
   int(*pipeArray)[2]; // array of arrays to store pipefd[2]'s for each command
   children = make_jrb();

   if (command->n_commands > 1)
   {
      pipeArray = (int(*)[2])malloc(sizeof(int[2]) * command->n_commands); // initialize pipe array
    
      for (int num = 0; num < command->n_commands; num++)
      {

         if (pipe(pipeArray[num]) < 0)
         { // child needs to write to fd[1] and and read from fd[0] (if not the first child)
            perror("pipe");
            exit(1);
         }
        
      }

      for (i = 0; i < command->n_commands; i++)
      {
      
         // flush all buffers
         fflush(stdin);
         fflush(stdout);
         fflush(stderr);
        
       
         pid = fork(); // call fork to create a space for child to run

         if (pid == 0) // if child, check if wait is true
         {
            setInOut(i, command, pipeArray);
           

            for (int l = 0; l < command->n_commands; l++)
            {
             
               close(pipeArray[l][0]);
               close(pipeArray[l][1]);
            }

            l = execvp(command->argvs[i][0], command->argvs[i]); // execute command
            perror(command->argvs[i][0]);
            exit(1);
         }
         else
         { //parent
            if(command->wait) jrb_insert_int(children, pid, new_jval_v(pipeArray[i]));
             // inserting the child's PID
         }
      }

      // parent
      if (command->wait && pid != 0)
      { // once all children have been forked
         while (!jrb_empty(children)) // wait on them and delete them from the jrb
         {
            k = wait(&status);
            tmp = jrb_find_int(children, k);
           
            if (tmp == NULL)
            {
               // child not in tree anymore
      
            }
            else
            { // only wait and delete if child is in the tree
               int *arr = (int *)tmp->val.v;
            
               jrb_delete_node(tmp); // delete child from tree
                close(arr[0]);
                close(arr[1]);
            }
         }
      }
      else
      {
         for(int i = 0; i < command->n_commands; i++) { //close out pipeFDs
            close(pipeArray[i][0]);
            close(pipeArray[i][1]);
         }
         // don't wait
      }
   }
   else
   { // single command
      fflush(stdin);
      fflush(stdout);
      fflush(stderr);
      int fd0, fd1;
      pid = fork();
      if (pid > 0)
      {                          // parent
         if (command->wait == 1) // don't think I need this for a single command, but you could still call NOWAIT on anything
         {                       // need to wait until child finishes
         
            while (k != pid)
            {
               k = wait(&status);
            }
         }
         else
         {
            // don't wait for children to finish
         }
      }
      else
      { // child

         if (command->stdin != NULL)
         { // redirect stdin
          
            fd0 = open(command->stdin, O_RDONLY); // open file
            if (fd0 < 0)
            {
               perror("failed opening input file - 30");
               exit(1);
            }
            if (dup2(fd0, 0) != 0)
            { // call dup2 on file with 0 to redirect stdin
               perror("failed dup for input file - 35");
               exit(1);
            }
            close(fd0);
         }

         if (command->stdout != NULL)
         { // Redirect stdout
            if (command->append_stdout == 0)
            {
               fd1 = open(command->stdout, O_WRONLY | O_TRUNC | O_CREAT, 0644); // Open file
            }
            else
            {
               fd1 = open(command->stdout, O_WRONLY | O_APPEND | O_CREAT, 0644); // Open file in append mode
            }
            if (fd1 < 0)
            {
               perror("failed opening stdout");
               exit(2);
            }
            if (dup2(fd1, 1) != 1)
            { // Redirect stdout to file
               perror("failed dup for output file");
               exit(1);
            }
            close(fd1); // Close file descriptor
         }
         l = execvp(command->argvs[0][0], command->argvs[0]);
         perror(command->argvs[0][0]);
         exit(1);
      }
   }
}

void printCommand(Command *command)
{
   if (command->stdin != NULL)
   {
      printf("\nstdin: %s\n", command->stdin);
   }
   else
   {
      printf("stdin: None\n");
   }
   if (command->stdout != NULL)
   {
      printf("stdout: %s  (append=%d)\n", command->stdout, command->append_stdout);
   }
   else
   {
      printf("stdout: None  (append=%d)\n", command->append_stdout);
   }
   printf("n_commands: %d\n", command->n_commands);
   printf("wait: %d\n", command->wait);
   for (int i = 0; i < command->n_commands; i++)
   {
      printf("%d: argc: %d   argv:", i, command->argcs[i]);
      for (int j = 0; j < command->argcs[i]; j++)
      {
         printf(" %s", command->argvs[i][j]);
      }
      printf("\n");
   }
   printf("\n");
}

int main(int argc, char *argv[])
{
   int numLines = 0;
   int index = 0;
   IS is;
   int ready = 0;
   int print = 0;
   int noCom = 0;
   int i;
   int finished = 1;
   char *in, *out;
   is = new_inputstruct(NULL);

   if (argv[1] != NULL)
   { // set booleans
      for (i = 0; i < strlen(argv[1]); i++)
      {
         if (argv[1][i] == 'r')
         {
            ready = 1;
           
         }
         else if (argv[1][i] == 'p')
         {
            print = 1;
           
         }
         else if (argv[1][i] == 'n')
         {
            noCom = 1;
          
         }
      }
   }


   Command *command = (Command *)malloc(sizeof(Command)); // data structure for storing and printing
   if (ready == 1)
   {
      printf("READY\n");
   }

   setCommand(command);
   // assign all values in command
   while (get_line(is) >= 0)
   {
      if(is->NF == 0) {
         continue;
      }


      if (strcmp(is->fields[0], ">") != 0 &&
          strcmp(is->fields[0], "<") != 0 &&
          strcmp(is->fields[0], ">>") != 0 &&
          strcmp(is->fields[0], "NOWAIT") != 0 &&
          strcmp(is->fields[0], "END") != 0)
      {
         command->argcs[index] = is->NF;
         char **args = malloc(200 * sizeof(char *));
         for (i = 0; i < is->NF; i++)
         {                                   // add the argcs and argvs
            args[i] = strdup(is->fields[i]); // insert
         }
         args[is->NF] = NULL; // null terminate array
         command->argvs[index] = args;
         command->n_commands++;
         index++;
      }
      else
      {
         if (strcmp(is->fields[0], "END") == 0)
         {
            if (print == 1)
            {
               printCommand(command);
            }

            execute(command);
            setCommand(command);
            index = 0;
            if (ready == 1)
            {
               printf("\nREADY\n");
            }
         }
         else if (strcmp(is->fields[0], "NOWAIT") == 0)
         {
            command->wait = 0;
         }
         else if (strcmp(is->fields[0], ">") == 0 || strcmp(is->fields[0], ">>") == 0)
         {
            command->stdout = strdup(is->fields[1]);
            if (strcmp(is->fields[0], ">>") == 0)
            {
               command->append_stdout = 1;
            }
         }
         else if (strcmp(is->fields[0], "<") == 0)
         {
            command->stdin = strdup(is->fields[1]);
         }
      }
   }
   freeCommand(command);
   jettison_inputstruct(is);
   return 0;
}