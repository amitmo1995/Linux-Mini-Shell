/*
 linuxMiniShell
 shell get command from the user as input and execute the command till the user
 input "done" command.
 this program support ^z and fg also
 */
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
const int MAX_INPUT=510;
int wordsCount(const char []);
void getDetails(char*[],char [],int);
char* subString(const char [],int,int);
int execution(char*[],int);
void clear(char* words[], int);
void runWithPipe(char* inStr,int firstPipeInd);
int lastProcStopped=0;//to keep the last stopped by ctrl Z process id (for the fg knows which process to put foreground)

int main() {
    signal(SIGTSTP,SIG_IGN);//to ignore the ^z difult handling in the "mini shell"
    int pipe_fd1[2];
    char currDir[MAX_INPUT];
    if (getcwd(currDir, sizeof(currDir)) == NULL)//innisialize the currDir variable with the current working directory if doesnt succeed prints error
        perror("getcwd() error");
    int firstpipeInd,secondPipeInd,pipesCounter=0;
    pid_t navigator1,navigator2;//to know if we are in sons proc or parent proc
    int numOfCommands = 0;//for the analyze in the end
    int wordsNum;//to keep the num of words of each command
    struct passwd *user;//to get user name
    int pipeFlag=0;//o means there is no pipe in the command otherwise 1 (if there is one pipe) or 2 (if there are two pipes)
    if ((user = getpwuid(getuid())) == NULL) {//check if success
        printf("ERR user name !\n");
        exit(1);
    }
    char inStr[MAX_INPUT + 1];//input string
    //run endless loop till gets the string "exit" (pure "exit" without spaces)
    do {
        signal(SIGTSTP,SIG_IGN);
        printf("%s@", user->pw_name);//prints the prompt
        printf("%s>", currDir );//prints the prompt
        if (fgets(inStr, MAX_INPUT, stdin) == NULL)//if the fgets function didnt work well start again!
            continue;
        for(int i=0;i< strlen(inStr);i++)//to ignore the " char for example echo "hi os" | wc -c will return 5 and not 7
        {
            if(inStr[i]=='"')
                inStr[i]=' ';
        }
        inStr[strlen(inStr) - 1] = '\0';//the last character is null terminator
        for(int i=0;i< strlen(inStr);i++)//check if there are pipes and where
        {
            if(inStr[i]=='|')
            {
                pipeFlag++;
                if(firstpipeInd==0)
                    firstpipeInd=i;
                else
                    secondPipeInd=i;
            }
        }
        if(pipeFlag>0)
        {
            if(pipeFlag==1)//run the command include the pipe by using runWithPipe function
            {
                runWithPipe(inStr,firstpipeInd);
                pipesCounter++;
                numOfCommands++;
            }
            if(pipeFlag==2)//run the two first commands include the first pipe by using runWithPipe function and direct the output by pipe to the last command
            {
                int rightWordsNum;
                char left[MAX_INPUT];
                char right[MAX_INPUT];
                char* l=subString(inStr,0,secondPipeInd);//substring return pointer to char and i want to free the pointer so i use l and r to free them in the end
                char* r=subString(inStr,secondPipeInd+1, (int)strlen(inStr));
                strcpy(left,l);
                strcpy(right, r);
                rightWordsNum=wordsCount(right);
                char* rightComm[rightWordsNum + 1];//+1 for the null (execvp needs array that ends with null
                getDetails(rightComm,right, rightWordsNum);
                if ((navigator1 = fork()) < 0)//if fork didnt succeed exit!
                {
                    clear(rightComm,rightWordsNum);
                    free(r);
                    free(l);
                    perror("Err fork !\n");
                    exit(1);
                } else if (navigator1 == 0)//sons process
                {
                    if (pipe(pipe_fd1) == -1)
                    {
                        perror("cannot open pipe");
                        exit(EXIT_FAILURE) ;
                    }
                    if ((navigator2 = fork()) < 0)//if fork didnt succeed exit!
                    {
                        perror("Err fork !\n");
                        exit(1);
                    }
                    else if(navigator2==0)//left
                    {
                        close(pipe_fd1[0]);
                        if(dup2(pipe_fd1[1], STDOUT_FILENO) == -1)//replace the output channel to the pipe
                        {
                            perror("err duping \n");
                            exit(1);
                        }
                        close(pipe_fd1[1]);
                        runWithPipe(left,firstpipeInd);//runs the two left commands
                        exit(0);
                    }
                    else//right , directing the output of the last two commands to the input of the last command and executing
                    {
                        wait(NULL);
                        close(pipe_fd1[1]);
                        if(dup2(pipe_fd1[0], STDIN_FILENO) == -1)//replace the input channel to the pipe
                        {
                            perror("err duping \n");
                            exit(1);
                        }
                        close(pipe_fd1[0]);
                        exit(execution(rightComm,rightWordsNum));
                    }
                } else//fathers process
                {
                    wait(NULL);
                }
                free(r);
                free(l);
                clear(rightComm,rightWordsNum);
                pipesCounter+=2;
                numOfCommands++;
            }
            if(pipeFlag>2)
                printf("more than 2 pipes not supported (Yet) \n");
        }
        else {
            numOfCommands++;//each input counts
            wordsNum = wordsCount(inStr);//wordsNum contains how many words there are in inStr
            if (wordsNum == 0)//if the user entered an empty string so continue and run the loop again
                continue;
            char *words[wordsNum + 1];//+1 for the null (because using exevp function)
            getDetails(words, inStr, wordsNum);//words will contain all the words the user inputed
            if (strcmp(words[0], "cd") == 0)//according to the ex rules
            {
                printf("command not supported (Yet) \n");
                clear(words, wordsNum);
                continue;
            }
            if (strcmp(words[0], "done") == 0 && wordsNum == 1)//just pure done finishes the program
            {
                clear(words, wordsNum);
                break;
            }
            if(strcmp(words[0], "fg") == 0 && wordsNum == 1)
            {
                if(waitpid(-1,NULL,WNOHANG)==0) {//check if there is some process that stopped
                    if(kill(lastProcStopped, SIGCONT)==-1)//sends sigcont to the last stopped process
                        perror("kill error! \n");
                    waitpid(-1,NULL,WUNTRACED);//wait for the process to stop or done
                    continue;
                }
                else {//if there is no process that zombie print bash: fg: current: no such job!
                    printf("bash: fg: current: no such job \n");
                    continue;
                }
            }
            //start forking and execute commands
            if ((navigator1 = fork()) < 0)//if fork didnt succeed exit!
            {
                perror("Err fork !\n");
                clear(words, wordsNum);
                exit(1);
            } else if (navigator1 == 0)//sons process
            {
                signal(SIGTSTP,SIG_DFL);//default handling sigtstp signal by the child
                exit(execution(words,wordsNum));//execution returns 0 if success and 1 otherwise and by the value 0|1 exit the proc
            } else//fathers process
            {//clears finished processes
                waitpid(-1,NULL,WUNTRACED);
                if(waitpid(navigator1,NULL,WNOHANG)==0) {//check if the son process stopped
                    lastProcStopped = navigator1;//keep the pid of the stopped process using global variable
                }

            }
            clear(words, wordsNum);//clears all mallocs in the current iteration
        }
        pipeFlag=0;
        firstpipeInd=0;
        secondPipeInd=0;
    }while (0 != 1);//runs till the break (input "done")
    //prints analyze and finish the program
    printf("Num of commands : %d\n",numOfCommands-1);//minus one because we dont count the done command(as exercise rules )
    printf("Num of pipes : %d\n",pipesCounter);
    printf("see you next time ! \n");
}

//count and return how many words there in the input string
int wordsCount(const char inStr[])
{
    int ind=0;
    int wordsCounter=0;
    int checker=0;//flags when the string inStr[i] is not space
    //check how many words in the string
    while(inStr[ind]!='\0')
    {
        if(inStr[ind]==' '||inStr[ind]=='\n')
        {
            checker=0;
            ind++;
        }
        else
        {
            if(checker==0)
                wordsCounter++;
            checker=1;
            ind++;
        }
    }
    return wordsCounter;
}
//returns substring from start index to finish index
char* subString(const char strIn[],int start,int finish)//V
{
    char* res=(char*)malloc((finish-start)*sizeof(char));
    if(res!=NULL) {//to check the malloc function
        for (int i = 0; i < (finish - start); i++) {
            res[i] = strIn[start + i];
        }
        res[finish - start] = '\0';
        return res;
    }
    else
    {
        perror("malloc error!!!");
        exit(1);
    }
}
//res will be the result array which contains details about the input string
void getDetails(char* res[],char inStr[MAX_INPUT],int wordsCount)
{
    int i=0,l,r,wordsInd=0;
    while(inStr[i]!='\0'&&inStr[i]!='\n'&&wordsInd<wordsCount&&i<strlen(inStr))
    {
        while(inStr[i]==' '||inStr[i]=='\0'||inStr[i]=='\n')
            i++;
        l=i;//l is start index of the word
        while(inStr[i]!=' '&&inStr[i]!='\0'&&inStr[i]!='\n')
            i++;
        r=i;//r is the finish index of the word
        res[wordsInd]=subString(inStr,l,r);
        wordsInd++;
    }
    res[wordsInd]=NULL;//the last cell have to be null for the execvp function
}
//executes the inputed commands
int execution(char* words[],int wordsNum)
{
    if((execvp(words[0], words))==-1) //will run the command if its real command
    {
        printf("ERR command not found!\n");
        clear(words, wordsNum);
        return 1;
    }
    clear(words, wordsNum);
    return 0;
}
//clears dynamic all the memory
void clear(char **words, int wordsNum)
{
    for(int i=0;i<wordsNum+1;i++)
    {
        free(words[i]);
        words[i]=NULL;
    }
    words=NULL;
}
//gets string which has one pipe inside and execute the commends by pipe between them
void runWithPipe(char* inStr,int firstPipeInd)
{
    int navigator1,navigator2;
    int pipe_fd1[2];//to direct the pipe
    int leftWordsNum,rightWordsNum;//to cut the big command into two commands one before the pipe and one after
    char left[MAX_INPUT];
    char right[MAX_INPUT];
    char* l=subString(inStr,0,firstPipeInd);
    char* r=subString(inStr,firstPipeInd+1, (int)strlen(inStr));
    strcpy(left,l);
    strcpy(right, r);
    leftWordsNum=wordsCount(left);
    rightWordsNum=wordsCount(right);
    char* leftComm[leftWordsNum+ 1];//plus one for the null to execute the command
    char* rightComm[rightWordsNum+ 1];
    getDetails(leftComm,left, leftWordsNum);
    getDetails(rightComm,right, rightWordsNum);
    if ((navigator1 = fork()) < 0)//if fork didnt succeed exit!
    {
        clear(leftComm, leftWordsNum);//clears mallocs
        clear(rightComm, rightWordsNum);
        free(r);
        free(l);
        printf("Err fork !\n");
        exit(1);
    } else if (navigator1 == 0)//sons process
    {
        if (pipe(pipe_fd1) == -1)
        {
            perror("cannot open pipe");
            exit(EXIT_FAILURE) ;
        }
        if ((navigator2 = fork()) < 0)//if fork didnt succeed exit!
        {
            perror("Err fork !\n");
            exit(1);
        }
        else if(navigator2==0)//left
        {
            close(pipe_fd1[0]);
            if(dup2(pipe_fd1[1], STDOUT_FILENO) == -1)//replace the output channel to the pipe
            {
                perror("err duping \n");
                exit(1);
            }
            close(pipe_fd1[1]);
            exit(execution(leftComm,leftWordsNum));
        }
        else//right
        {
            wait(NULL);
            close(pipe_fd1[1]);
            if(dup2(pipe_fd1[0], STDIN_FILENO) == -1)//replace the input channel to the pipe
            {
                perror("err duping \n");
                exit(1);
            }
            close(pipe_fd1[0]);
            exit(execution(rightComm,rightWordsNum));
        }
    } else//fathers process
    {
        wait(NULL);
    }
    free(r);
    free(l);
    clear(leftComm, leftWordsNum);//clears mallocs
    clear(rightComm, rightWordsNum);
}
