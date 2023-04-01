/***************************
 * SMALLSH ASSIGNEMENT 3
 * SAI BHAVEESH BEEMIREDDY
 * 933916125
 * ************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

bool foreground = true;
bool skip = true;



void getSIGINT(int);
void CurrentStatus(int);
void getSIGSTP(int);
/***************************
** * Function: createPID
** * Description: replaces the "$$" for the PID
**********************************************************************/

char* createPID(int PID, char* commandLineArg, char* original)
{
    
    int pidLength;
    
    char pid[100];
    char *newPID;

    sprintf(pid, "%d", PID);                    

    pidLength = strlen(pid);
    int Length = strlen(original);

    int i;
    int count = 0;
    int index = 0;
    for(i = 0; commandLineArg[i] != '\0'; i++)                                 
    {
      if(strstr(&commandLineArg[i], original) == &commandLineArg[i])            
      {
        count++;
        i += Length - 1;
      }
    }

    newPID = (char*)malloc(i + count * (pidLength - Length) + 1);

    while(*commandLineArg)                                               
    {
      if(strstr(commandLineArg, original) == commandLineArg)
      {
        strcpy(&newPID[index], pid);                 //Getting the PID to end of string
        index += pidLength;
        commandLineArg += Length;
      }
      else                                                        
      {
        newPID[index++] = *commandLineArg++;
      }
    }

    newPID[index] = '\0';

    return newPID;                                                        
}


int main(int argc, char *argv[])
{
  char userInput[2048];
  bool valid = true;
  int Input;
  int Output;
  int status = 0;
  int i = 0;
  bool process = false;
  int currentPID;
  int index;
  char* parsedString;
  char* UsercommandLine[2048];
  char newString[512];
  char comment[100];
  char* pidCheck;
  pid_t NewPID = -5;
  struct sigaction SIGINT_action;                 //https://www.gnu.org/software/libc/manual/html_node/Sigaction-Function-Example.html                
  struct sigaction SIGSTP_action;

  SIGINT_action.sa_handler = getSIGINT;                                      
  sigfillset(&(SIGINT_action.sa_mask));
  SIGINT_action.sa_flags = 0;

  SIGSTP_action.sa_handler = getSIGSTP;                                      
  SIGSTP_action.sa_flags = 0;

  sigaction(SIGINT, &SIGINT_action, NULL);                                     
  sigaction(SIGTSTP, &SIGSTP_action, NULL);

  while(valid == true)                                                             
  {
    printf(": ");     //cmd line prompt :
    fflush(stdout);
    fgets(userInput, 512, stdin);                                              
    parsedString = strtok(userInput, " ");                                      

    index = 0;
    char input[10000] = {0};
    char output[10000] = {0};

    while(parsedString != NULL)
    {                                                                          
      if(strcmp(parsedString, "<") == 0)                       //checks for < 
      {
        parsedString = strtok(NULL, " ");                                      
        sscanf(parsedString, "%s", input);
        parsedString = strtok(NULL, " ");
      }
      else if(strcmp(parsedString, ">") == 0)                    //checks for >
      {
        parsedString = strtok(NULL, " ");                                      
        sscanf(parsedString, "%s", output);
        parsedString = strtok(NULL, " ");
      }
      else if(strstr(parsedString, "$PD") != NULL)                //checks for $PD to replace with PID
      {
        sscanf(parsedString, "%s", newString);                                
        UsercommandLine[index++] = strdup(createPID(getpid(), newString, "$PD"));
        parsedString = strtok(NULL, " ");
      }
      else if(strcmp(parsedString, "$HM") == 0)                    //checks for $HM
      {
        chdir(getenv("HOME"));
      }
      else
      {
        sscanf(parsedString, "%s", newString);                                
        UsercommandLine[index] = strdup(newString);
        parsedString = strtok(NULL, " ");
        index++;                                                           
      }
    }

    strcpy(comment, (UsercommandLine[0]));                                   
    if(userInput[0] == '\n')                                                    
    {
      UsercommandLine[0] = strdup(NULL);
    }

    if(strcmp(UsercommandLine[index - 1], "&") == 0)          //looks for &
    {
      UsercommandLine[index - 1] = NULL; //ignores the &
      process = true;
    }
    else
    {
      UsercommandLine[index] = NULL;
      process = false;
    }

    if(strcmp(UsercommandLine[0], "exit") == 0)  //exit the shell
    {
      valid = false;
      exit(0);
    }
    else if(strcmp(UsercommandLine[0], "status") == 0)                  //Status is called
    {
      CurrentStatus(status);
    }
    else if(strcmp(UsercommandLine[0], "cd") == 0)              
    {
      if (UsercommandLine[1]) {
				if (chdir(UsercommandLine[1]) == -1) {
					chdir(getenv("HOME"));
				}
			} else {
        //print error to STDERR
        fprintf(stderr,"ERROR NO DIR");
			}
    }
    else if(strcmp(UsercommandLine[0], "#") == 0)
    {                                                                          
      //comment do nothing
    }
    else if(strcmp(UsercommandLine[0], "echo") == 0 && UsercommandLine[1] != NULL && strcmp(UsercommandLine[1], "$PD") == 0)
    {
      printf("%d\n", getpid());
    }
    else if(skip == false)                                                       
    {
      skip = true;
    }
    else
    {
      NewPID = fork();                                              

      switch(NewPID)
      {
        case -1:                //Error case
        {                                                                       
          perror("Error\n");
          fflush(stdout);
          status = 1;
          break;
        }
        case 0:                                                                 
        {                                                                       
          if((process == false) || (foreground == false))
          {
            SIGINT_action.sa_handler = SIG_DFL;                                 
            sigaction(SIGINT, &SIGINT_action, NULL);
          }

          if(input[0] != 0)                        //checking if the input file is valid
          {
            Input = open(input, O_RDONLY);                             
            if(Input == -1)
            {
              printf("can't open %s for input!\n", input);               
              fflush(stdout);
              _exit(1);
            }
            if(dup2(Input, 0) == -1)                   //file invalid      
            {
              perror("INVALID INPUT!\n");
              fflush(stdout);
              _exit(1);
            }
            close(Input);                                                  
          }

          if(output[0] != 0)                                                
          {
            Output = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0744); //Reads, Creates, Truncates, Permissions
                                                                               
            if(Output == -1)                               //can't open file
            {
              printf("can't open %s for output\n", output);
              fflush(stdout);
              _exit(1);
            }
            if(dup2(Output, 1) == -1)                   //file not valid
            {
              perror("VALID OUTPUT!\n");
              fflush(stdout);
              _exit(1);
            }
            close(Output);                                                 
          }

          if(execvp(UsercommandLine[0], UsercommandLine) < 0)                           
          {                                                                    
            printf("%s: no such file or directory\n", UsercommandLine[0]); //if file doesn't exist in directory 
            fflush(stdout);
            _exit(1);
          }
          break;
        }
        default:                                                                
        {                                                                       
          if(process == false || foreground == false)          //checks mode
          {
            waitpid(NewPID, &status, 0);                                 
          }
          else
          {
            printf("background pid is %i\n", NewPID);                        
            fflush(stdout);
            usleep(2000);
          }
          break;
        }
      }
    }
    usleep(2000);
    NewPID = waitpid(-1, &status, WNOHANG);    //WNOHANG so child can run while parent waits                             
    while(NewPID > 0)
    {
      printf("background pid %i is done: ", NewPID);                          
      fflush(stdout);                                                        
      CurrentStatus(status);             //exit staus
      NewPID = waitpid(-1, &status, WNOHANG);
    }
    
  }
}



/**************************************
** * Function: getSIGINT
** * Description: CTRL-C Handler
**************************************/

void getSIGINT(int signo)
{
  if(foreground == true)              //checking if we are in foreground mode or not
  {
    printf("\n");                                                               
    fflush(stdout);
    skip = false;
  }
  else
  {
    printf("\n");                                                               
    fflush(stdout);
    skip = false;
  }
}


/*********************************
** * Function: getSIGSTP
** * Description: CTRL-Z Handler
*********************************/

void getSIGSTP(int signo)
{
  if(foreground == true)                          //checking if we are in foreground mode or not
  {
    foreground = false;                                                      
    write(1, "\nEntering foreground-only mode\n", 51);
    //sleep(2);
    skip = false;                                                                   
  }
  else
  {
    foreground = true;                                                      
    write(1, "\nExiting foreground-only mode\n", 31);
    skip = true;                                                                   
  }
}

/***************
** * Function: Currentstatus
** * Description: Gives the current status foreground process
******************************************************************/

void CurrentStatus(int status)
{
                                                      
  if(!WIFEXITED(status))                                                       
  {
    printf("terminated by signal %i\n", status);
    fflush(stdout);
  }
  else                                                                       
  {
    int exit = WEXITSTATUS(status);
    printf("exit value %i\n", exit);
    fflush(stdout);
  }
}





