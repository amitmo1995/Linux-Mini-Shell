# Linux-Mini-Shell

program name : linuxMiniShell
student name : Amit moshe

explanation about the program :
the program gets as input command from the user and executing the command till 
the input is "done" .
this is also support ^z and fg by handling with signals
all the functions are still the same as ex3 
in the end of the program there will be analyze of the commands the user puted.
(cd doesnt supported yet as the exercise rules and fg will continue just the last stopped process as the ex rules)
(if the user puts fg while there is no background process it will print output same as linux original terminal output in this case)


main functions : 
wordsCount : counts how many words there in the input string and return .
                        the count working by check when there is spaces or \n's and flags when found , then run till find new character that is not space or \n 
                        and finish if find \0 in parallel the counter runs and count the words , in the end returns the counter variable.

subString : gets string , start index and finish index and return new dynamically assigned string that contains the substring from start index to finish index.
                   it works by copying the input sting from start to finish inex to the new dynamically assigned string. 

getDitails : gets strings array for the result (by reference) , the original inputed string and how many words there is in the original string and fill the res strings array with the words of the original inputed                            string.
                   it works by check where is the words starts , where they finish and call the substring function to create new dynamic memory and copy the substring from start index to finish index from the                               original string , then put it in the res strings array , it run till the last word of the original string and the res array contains all the words from the original string one by one .

execution : gets command as string and how many words the command contains and execute the command .

clear : get array with all the inputed strings and how many words the user have inputed and clears all mallocs each iteration of the program.


runWithPipe : gets pointer to char which is the inputed command with pipe and execute the two commands with piping between them .


files list : linuxMiniShell.c

how to compile : gcc linuxMiniShell.c -o linuxMiniShell

how to run : ./linuxMiniShell

input : string (not over 510 characters)

output : execution of the inputed commands and in the end prints stats about number of commands and number of pipes that the user inputed .
