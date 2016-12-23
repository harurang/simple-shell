/*
Author: Hillary Arurang
*/

#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>


#define BUFF_LENGTH 1024

/*
Creates a new child,
opens input and output,
and returns exectued command.

@params read, write, array of parsed command
@return executed command
*/
int spawn(int input, int output, char *argv[])
{
    pid_t pid;
    if ((pid = fork ()) == 0){
      // open input
      dup2 (input, 0);
      // open output
      dup2 (output, 1);
      // return executed command
      return execvp (argv[0], argv);
    }
    return pid;
}

/*
Removes escape characters in array.

@param array of user input
*/
void removeEscapeCharacters(char *userInput) {
    // remove new userInput escape characters
    int j = 0;
    for(j; j<BUFF_LENGTH; j++) {
      if(userInput[j] == '\n')
        userInput[j] = '\0';
    }
}

/*
Splits a user input into an array of strings.

@params user input and array to store tokenized user input
@returns 1 if there are pipes in the input and 0 if not
*/
int tokenizeInput(char *userInput, char *tokenizedInput[]) {
    int pipes = 0;
    char *token;
    // get string from userInput
    token = strtok(userInput," ");
    int i=0;
    while(token!=NULL){
      // if there is a pipe
      if((strcmp(token, "|") == 0))
        pipes = 1;
      // store string
      tokenizedInput[i]=token;
      // get next string
      token = strtok(NULL," ");
      i++;
    }
    // set last index to NULL
    tokenizedInput[i]=NULL;

    return pipes;
}

/*
Creates an new array for the first part of the user command
and opens appropriate input and output for remaining
commands.

@param array of user input
*/
void evaluateStreams(char* tokenizedInput[]) {
    int i, input, output = 0;
    // array for first command
    char *newArr[BUFF_LENGTH];
    // determines if we are parsing the first command or not
    int symbolReached = 0;

    while (tokenizedInput[i] != NULL) {
      if (strcmp(tokenizedInput[i], ">") == 0) {
        // open file
        output = open(tokenizedInput[i + 1], O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        //open output
        dup2(output, 1);
        //if we have reached a < or > there is no need to add to newArr
        symbolReached = 1;
      }
      else if (strcmp(tokenizedInput[i], "<") == 0) {
        // open file
        input = open(tokenizedInput[i + 1], O_RDONLY);
        // open input
        dup2(input, 0);
        // if we have reached a < or > there is no need to add to newArr
        symbolReached = 1;
      }
      if (symbolReached == 0) {
        newArr[i] = tokenizedInput[i];
      }
      i++;
      //set next index to null, if it is not the last index it will be overwritten
      newArr[i] = NULL;
      }
      //execute sys call
      execvp(newArr[0], newArr);
}

/*
Uses the same array by overwriting the array for each command.
Each command is piped.

@param tokenized array of user input
*/
void evaluatePipes(char *tokenizedInput[]) {
    int fd [2];
    char *newArr[BUFF_LENGTH];
    int i, j, input = 0;

    while(tokenizedInput[j]!=NULL) {
      if(strcmp(tokenizedInput[j], "|") == 0) {
        // set last index of array to NULL
        newArr[i] = NULL;
        //create pipe
        pipe(fd);
        spawn(input, fd[1], newArr);
        // close write
        close (fd[1]);
        // save read for next child
        input = fd [0];
        // reuse newArr for next command by overwritting
        i=0;
      }
      else {
        newArr[i] = tokenizedInput[j];
        i++;
      }
      j++;
    }
    // set last index of last array to NULL
    newArr[i] = NULL;
    // open input
    dup2 (input, 0);
    // sys call
    execvp (newArr[0], newArr);
}

/*
Retrieves and prints current directory
*/
void printCurrentDirectory() {
    // get current path each time
    char pwd[BUFF_LENGTH];
    getcwd(pwd, sizeof(pwd));
    // print path to terminal
    printf("%s\n", pwd);
}

/*
Evaluates tokenized user input.

@params tokenized user input, a 1 or 0 that
represent whether there are pipes or streams
*/
void evaluateInput(char *tokenizedInput[], int pipes, int streams) {
    // represents process id of forked process
    pid_t pid;

    // if user enters cd
    if(strcmp(tokenizedInput[0], "cd") == 0) {
        // move to appropriate directory
        chdir(tokenizedInput[1]);
    }
    // if user input contains pipes
    else if(pipes) {
      if((pid = fork()) == 0)
        evaluatePipes(tokenizedInput);
      else
        // wait for parent process to finish
        wait(NULL);
    }
    // if user input contains streams
    else if(streams) {
      if((pid = fork()) == 0)
        evaluateStreams(tokenizedInput);
      else
        wait(NULL);
    }
    // if user input contains neither streams nor pipes
    else {
      if((pid = fork()) == 0)
        execvp(tokenizedInput[0], tokenizedInput);
      else
        wait(NULL);
    }
}

int main() {
    // store user input into userInput
    char userInput[BUFF_LENGTH];
    // parse user input into tokenizeInput array
    char* tokenizedInput[BUFF_LENGTH];

    while(1) {
      // will determine if input contain pipes, streams or neither
      int pipes = 0;
      int streams = 0;

      printCurrentDirectory();

      // get user input and store in userInput
      if(!fgets(userInput, BUFF_LENGTH, stdin)){
        break;
      }

      removeEscapeCharacters(userInput);

      // if user input is 'exit', exit loop
      if(strcmp(userInput, "exit") == 0){
          break;
      }

      // tokenize userInput and store in tokenizedInput
      pipes = tokenizeInput(userInput, tokenizedInput);
      if(pipes == 0) {
          streams = 1;
      }

      // evaluate tokenized user input
      evaluateInput(tokenizedInput, pipes, streams);
    }
}
