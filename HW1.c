#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
//#include <signal.h>

#define MAX_NAME_LENGTH 50
#define MAX_GRADE_LENGTH 3
#define MAX_LINE_LENGTH (MAX_NAME_LENGTH + MAX_GRADE_LENGTH + 3) // name, grade, comma, space, newline

void writeToLog(const char *action) {
    pid_t pid = fork(); //fork new process
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { //child process
        int logFile = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);//open or create(doesnt exist) log file for writing
        if (logFile == -1) {
            perror("Error opening log file");
            exit(EXIT_FAILURE);
        }

        time_t currentTime;
        time(&currentTime);
        char logEntry[MAX_LINE_LENGTH + 30];//buffer for log entry
        sprintf(logEntry, "[%s] %s\n", strtok(ctime(&currentTime), "\n"), action);//format of log

        if (write(logFile, logEntry, strlen(logEntry)) == -1) {//write log to file
            perror("Error writing to log file");
            exit(EXIT_FAILURE);
        }
        close(logFile);
        exit(EXIT_SUCCESS); // Child process exits
    } else {        //parent proces
        wait(NULL);
    }
}


void addStudentGrade(const char *filename, const char *name, const char *grade) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {//child process
        int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }
        char entry[MAX_LINE_LENGTH];
        sprintf(entry, "%s, %s\n", name, grade);
        if (write(fd, entry, strlen(entry)) == -1) {//write entry to the file
            perror("Error writing to file");
            exit(EXIT_FAILURE);
        }
        close(fd);
        writeToLog("Added student grade");//write the state to the log 
        exit(EXIT_SUCCESS);
    } else {
        wait(NULL); //wait child to finish
    }
}



void searchStudent(const char *filename, const char *name) {

    //printf("name: %s\n", name); 

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {// Child process
        int found = 0;
        int file = open(filename, O_RDONLY);
        if (file == -1) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        char line[MAX_LINE_LENGTH];
        ssize_t bytesRead;
        while ((bytesRead = read(file, line, sizeof(line) - 1)) > 0) {
            line[bytesRead] = '\0'; 
            if (bytesRead > 0 && line[bytesRead - 1] == '\n') {//check for newline and replace with null 
                line[bytesRead - 1] = '\0';
            }
            char *found_name = strstr(line, name);
            if (found_name != NULL && strncmp(found_name, name, strlen(name)) == 0) {
                printf("%s\n", line); //print matched line
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("There is no student with this name.\n");
        }

        if (bytesRead == -1) {
            perror("Error reading from file");
        }
        close(file);
        writeToLog("Searched for student grade");//write the state to the log 
        exit(EXIT_SUCCESS);
    } else {
        wait(NULL); //wait child to finish
    }
}


void merge(char arr[][MAX_LINE_LENGTH], int l, int m, int r, int sortByGrade, int ascendingOrder) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
    char L[n1][MAX_LINE_LENGTH], R[n2][MAX_LINE_LENGTH]; // create temp arrays

    // copy data
    for (i = 0; i < n1; i++)
        strcpy(L[i], arr[l + i]);
    for (j = 0; j < n2; j++)
        strcpy(R[j], arr[m + 1 + j]);

    i = 0; // first subarray
    j = 0; // second subarray
    k = l; // merged subarray
    while (i < n1 && j < n2) {
        int comparison;
        if (sortByGrade) {
            char *gradeL = strchr(L[i], ',') + 2; // Extract grade from string
            char *gradeR = strchr(R[j], ',') + 2; // Extract grade from string
            comparison = ascendingOrder ? strcmp(gradeL, gradeR) : strcmp(gradeR, gradeL);
        } else {
            comparison = ascendingOrder ? strcmp(L[i], R[j]) : strcmp(R[j], L[i]);
        }

        if (comparison <= 0) {
            strcpy(arr[k], L[i]);
            i++;
        } else {
            strcpy(arr[k], R[j]);
            j++;
        }
        k++;
    }

    while (i < n1) { // copy the remaining elements of L
        strcpy(arr[k], L[i]);
        i++;
        k++;
    }
    while (j < n2) { // copy the remaining elements of R
        strcpy(arr[k], R[j]);
        j++;
        k++;
    }
}

void mergeSort(char arr[][MAX_LINE_LENGTH], int l, int r, int sortByGrade, int ascendingOrder) {//main merge sort call öerge functions half and half ı choose it because it is better for high sizes
    if (l < r) {
        int m = l + (r - l) / 2;

        // Sort first and second halves
        mergeSort(arr, l, m, sortByGrade, ascendingOrder);
        mergeSort(arr, m + 1, r, sortByGrade, ascendingOrder);

        // Merge the sorted halves
        merge(arr, l, m, r, sortByGrade, ascendingOrder);
    }
}

void sortAll(const char *filename, int sortByGrade, int ascendingOrder) {
    pid_t pid = fork();//child process handeling
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {//child process
        FILE *file = fopen(filename, "r+");
        if (!file) {
            perror("Error opening file for reading and writing");
            exit(EXIT_FAILURE);
        }
        int count = 0;
        char data[MAX_LINE_LENGTH][MAX_LINE_LENGTH];
        while (fgets(data[count], MAX_LINE_LENGTH, file)) {//read data into an array
            count++;
        }
        if (feof(file)) {
            clearerr(file);//clear the EOF flag for use it again
        } else {
            perror("Error reading from file");
            exit(EXIT_FAILURE);
        }
        fclose(file);
        mergeSort(data, 0, count - 1, sortByGrade, ascendingOrder);//use merge algo
        file = fopen(filename, "w");
        if (!file) {
            perror("Error opening file for writing");
            exit(EXIT_FAILURE);
        }

        //rewrite sorted names , grades 
        for (int i = 0; i < count; i++) {
            if (fprintf(file, "%s", data[i]) < 0) {
                perror("Error writing to file");
                exit(EXIT_FAILURE);
            }
        }
        fclose(file);
        writeToLog("Sorted all student grade");//write the state to the log 
        exit(EXIT_SUCCESS);
    } else {
        wait(NULL); //wait child to finish
    }
}


void showAll(const char *filename) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        char command[50];
        sprintf(command, "cat %s", filename);
        system(command);
        writeToLog("Displayed all student grades");//write the state to the log 
        exit(EXIT_SUCCESS);
    } else {
        wait(NULL); //wait child to finish
    }
}


void listGrades(const char *filename) {
    pid_t pid = fork();//child process handeling
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        int file = open(filename, O_RDONLY);
        if (file == -1) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        char line[200];
        int count = 0;
        const int numEntries = 5; //costant entries variable
        ssize_t bytesRead;

        while (count < numEntries) {
            bytesRead = read(file, line, sizeof(line));
            if (bytesRead <= 0) {
                break;//EOF or error
            }

            int i = 0;
            while (i < bytesRead) {//find \n characters 
                if (line[i] == '\n') {
                    write(STDOUT_FILENO, line, i + 1);//print the current line
                    count++;
                    if (count >= numEntries) {
                        break; //5 lines printed
                    }
                    //shift the remaining bytes
                    memmove(line, line + i + 1, bytesRead - i - 1);
                    bytesRead -= i + 1;
                    i = 0; //checking from start again
                } else {
                    i++;
                }
            }
            if (i > 0 && count < numEntries) {//save remaining bytes
                memmove(line, line + i, bytesRead - i);
            }
        }

        if (bytesRead == -1) {
            perror("Error reading from file");
        }
        close(file);
        writeToLog("Listed first 5 student grade");//write the state to the log 
        exit(EXIT_SUCCESS);
    } else {
        wait(NULL); //wait child to finish
    }
}





void listSome(int numEntries, int pageNumber, const char *filename) {
    pid_t pid = fork();//child process handeling
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {//child process
        int file = open(filename, O_RDONLY);
        if (file == -1) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }
        char line[300];
        ssize_t bytesRead;
        int offset = (pageNumber - 1) * numEntries;
        int currentLine = 0; 

        while (currentLine < offset) {//skip lines until reaching offset
            char c;
            while (read(file, &c, 1) > 0 && c != '\n'); //read until newline
            if (c == '\n') {
                currentLine++;
            }
        }
        int count = 0;
        while (count < numEntries) {
            bytesRead = read(file, line, sizeof(line));
            if (bytesRead <= 0) {
                break; // EOF or error
            } 
            int i = 0;
            while (i < bytesRead) {//find newline character
                if (line[i] == '\n') {//print the current line until the newline
                    write(STDOUT_FILENO, line, i + 1); //include the newline
                    count++;
                    if (count >= numEntries) {
                        break; //5 line orinted
                    } 
                    memmove(line, line + i + 1, bytesRead - i - 1);//shift the remaining bytes
                    bytesRead -= i + 1;
                    i = 0; //start checking from start again
                } else {
                    i++;
                }
            }
            if (i > 0 && count < numEntries) {//save remaining bytes
                memmove(line, line + i, bytesRead - i);
            }
        }

        if (bytesRead == -1) {
            perror("Error reading from file");
        }
        close(file);
        writeToLog("Listed some student grade");//write the state to the log 
        exit(EXIT_SUCCESS);
    } else {
        wait(NULL); //wait child to finish
    }
}

int tokenize(char *command, const char *delimiters, char *tokens[]) {//doesn't creating child processes.it is simple tokenize function
    int numTokens = 0;
    char *token = strtok(command, delimiters);
    while (token != NULL && numTokens < 10) {
        tokens[numTokens++] = token;
        token = strtok(NULL, delimiters);
    }
    tokens[numTokens] = NULL; 
    return numTokens;
}

int main(int argc, char *argv[]) {
    char command[MAX_LINE_LENGTH];

    printf("%s for getting info\n", argv[0]);

    while (fgets(command, sizeof(command), stdin)) {
        //remove \n 
        command[strcspn(command, "\n")] = '\0';
        //parse command 
        char *tokens[20]; 
        int numTokens = tokenize(command, " ", tokens);//seperate it by space
        pid_t pid = fork();//child process handeling
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { //child process
            if (numTokens == 1 && strcmp(tokens[0], "./student_grade") == 0) {
                printf("%s addStudentGrade <name> <grade>\n", argv[0]);
                printf("%s searchStudent <name>\n", argv[0]);
                printf("%s sortAll <filename> [name|grade] [asc|desc]\n", argv[0]);
                printf("%s showAll <filename>\n", argv[0]);
                printf("%s listGrades <filename>\n", argv[0]);
                printf("%s listSome <numEntries> <pageNumber> <filename>\n", argv[0]);
                printf("%s exit\n", argv[0]);
            }
            if (strcmp(tokens[1], "addStudentGrade") == 0) {
                if (numTokens < 4) {
                    printf("Usage: %s addStudentGrade <name> <grade>\n", tokens[0]);
                    exit(EXIT_FAILURE);
                }

                char combinedString[MAX_NAME_LENGTH];
                combinedString[0] = '\0';  //empty string
                for (int i = 2; i < numTokens - 1; i++) {
                    strcat(combinedString, tokens[i]);
                    if (i < numTokens - 1) {  //add spaces 
                        strcat(combinedString, " ");
                    }
                }
                addStudentGrade("grades.txt", combinedString, tokens[numTokens - 1]);
            } else if (strcmp(tokens[1], "searchStudent") == 0) {
                if (numTokens < 3) {
                    printf("Usage: %s searchStudent <name>\n", tokens[0]);
                    exit(EXIT_FAILURE);
                }

                char combinedString2[MAX_NAME_LENGTH];
                combinedString2[0] = '\0';  //empty string
                for (int i = 2; i < numTokens; i++) {
                    strcat(combinedString2, tokens[i]);
                    if (i < numTokens) {  //add spaces 
                        strcat(combinedString2, " ");
                    }
                }
                printf("%s\n",combinedString2);
                searchStudent("grades.txt",combinedString2);//call searchStudent

            } else if (strcmp(tokens[1], "sortAll") == 0) {
                if (numTokens != 5) {
                    printf("Usage: %s sortAll <filename> [name|grade] [asc|desc]\n", tokens[0]);
                    continue;
                }
                //printf("%s",tokens[2]);
                int sortbygrade = (strcmp(tokens[3],"grade")==0);
                int ascendingorder = (strcmp(tokens[4],"asc")==0);
                sortAll("grades.txt",sortbygrade,ascendingorder);//call sortAll 

            } else if (strcmp(tokens[1], "showAll") == 0) {
                if (numTokens != 3) {
                    printf("Usage: %s showAll <filename>\n", tokens[0]);
                    continue;
                }
                showAll(tokens[2]);//call showAll 

            } else if (strcmp(tokens[1], "listGrades") == 0) {
                if (numTokens != 3) {
                    printf("Usage: %s listGrades <filename>\n", tokens[0]);
                    continue;
                }
                listGrades("grades.txt");//call listGrades 
            } else if (strcmp(tokens[1], "listSome") == 0) {
                if (numTokens != 5) {
                    printf("Usage: %s listSome <numEntries> <pageNumber> <filename>\n", tokens[0]);
                    continue;
                }
                int numEntries = atoi(tokens[2]);//atoi for int
                int pageNumber = atoi(tokens[3]);
                listSome(numEntries, pageNumber, "grades.txt");//Call listSome 
                //listSome(tokens[2],tokens[3],tokens[4]);//Call listSome 
            } else if (strcmp(tokens[1], "exit") == 0) {
                break; //exit
            } else {
                printf("Unknown command: %s\n", tokens[1]);
            }

            //reset tokens
            for (int i = 0; tokens[i] != NULL; i++) {
                memset(tokens[i], 0, strlen(tokens[i]) + 1); //reset string
            }
            numTokens = 0; //reset count
        }
    }
    wait(NULL); //wait child to finish
    return EXIT_SUCCESS;
}