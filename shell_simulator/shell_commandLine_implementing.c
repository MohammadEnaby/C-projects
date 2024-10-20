#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

int MAX_WORDS = 6;
#define MAX_COMMAND_LENGTH 1024

typedef struct {
    char* command;
    char* newCom;
} map;

typedef struct {
    char* command;  // Command string
    pid_t pid;      // Process ID
} job;

void updatePrompt(int cmdCount, int aliasCount, int scriptLines, char *prompt);
char** splitSentenceIntoWords(char* sentence, int* total);
void addAlias(map** alias, int* aliasCount, char* input, int* total, int* cmd);
void printAliases(map* alias, int aliasCount);
void removeAlias(map** alias, int* aliasCount, char* input);
void readScript(char* fileName, int* lines, map** alias, int* aliasCount, int* cmd, int* total);
void freeArguments(char** args);
void freeAliases(map* alias, int aliasCount);
char** replaceAliasWithCommand(map* alias, int aliasCount, char** argv, int* total);

int execute_command(char* cmd,int* total);

int exec_in_background(job** jobs, char* command, int* num_of_processes,int* total);
void check_if_end(job** jobs, int* num_of_processes,int* cmd);
void delete_process(job** jobs, int index, int* num_of_processes);
void print_all_jobs(job* jobs, int num_of_jobs);
void freeJobs(job* jobs, int num_of_jobs);

int* Count_AND_OR(char** args, int* commandCount);
char** separate_input_AND_OR(char* input, int commandsCount);
void manage_AND_OR(char** args, const int* operations, int cmdCount, int* status,int* total,int op,int* cmd,job** jobs,int* num_of_jobs);

void updatePrompt(int cmdCount, int aliasCount, int scriptLines, char *prompt) {
    sprintf(prompt, "#cmd:%d|#alias:%d|#script lines:%d> ", cmdCount, aliasCount, scriptLines);
}

char** splitSentenceIntoWords(char* sentence, int* total) {
    int max_words=MAX_WORDS;
    if (strstr(sentence,"&&")!=NULL||strstr(sentence,"||")!=NULL){
        max_words=50;
    }

    char** words = (char**)malloc((max_words + 1) * sizeof(char*));
    if (words == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    int wordCount = 0;

    char* p = sentence;


    while (*p != '\0' && wordCount < max_words) {
        // Skip any leading spaces
        while (*p == ' ') {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        char* start;
        char* end;
        if (*p == '\'' || *p == '"') {
            // Handle quoted string
            char quote = *p;
            start = ++p;
            while (*p != quote && *p != '\0') {
                p++;
            }
            if (*p == '\0') {
                printf("ERR: brackets are not balanced\n");
                return NULL;
            }
            (*total)++;
            if (strstr(sentence,"&&")!=NULL||strstr(sentence,"||")!=NULL){
                (*total)--;
            }
            end = p++;
        } else {
            // Handle regular word
            start = p;
            while (*p != ' ' && *p != '\0' && *p != '\'' && *p != '"') {
                p++;
            }
            end = p;
        }

        // Calculate the length of the word
        size_t length = end - start;
        words[wordCount] = (char*)malloc(length + 1);
        if (words[wordCount] == NULL) {
            perror("malloc");
            freeArguments(words);
            return NULL;
        }

        // Copy the word and null-terminate it
        strncpy(words[wordCount], start, length);
        words[wordCount][length] = '\0';

        wordCount++;
    }
    words[wordCount] = NULL;

    if (strstr(sentence,"&&")!=NULL||strstr(sentence,"||")!=NULL){
        int count=0;
        int i=0;
        for (int j=0; words[j]!=NULL;j++){
            while (strcmp(words[i],"&&")!=0 && strcmp(words[i],"||")!=0){
                count++;
                i++;
                if (count>=max_words){
                    freeArguments(words);
                    printf("ERR: Arguments more than the limit\n");
                    return NULL;
                }
            }
            count=0;
        }
        words[wordCount]=NULL;
        return words;
    }


    if (wordCount >= max_words) {
        freeArguments(words);
        fprintf(stderr,"ERR: Arguments more than the limit\n");
        return NULL;
    }

    return words;
}

void addAlias(map** alias, int* aliasCount, char* input, int* total, int* cmd) {
    if (alias == NULL || input == NULL) return;

    char* aliasInput = input + 6; // Assuming input starts with "alias "
    char* equalsSign = strchr(aliasInput, '=');

    if (equalsSign == NULL) {
        printf("ERR: alias: Invalid format\n");
        return;
    }

    // Check if there's space around the equals sign
    if (*(equalsSign - 1) == ' ' || *(equalsSign + 1) == ' ') {
        printf("ERR: alias: Invalid format\n");
        return;
    }

    // Extract new command name
    char* newCom = strndup(aliasInput, equalsSign - aliasInput);

    if (newCom == NULL) {
        printf("ERR: alias: Memory allocation failed\n");
        return;
    }

    // Check if newCom contains space
    if (strchr(newCom, ' ') != NULL) {
        printf("ERR: alias: Invalid format\n");
        free(newCom);
        return;
    }

    // Extract the command to be aliased
    char* commandStart = equalsSign + 1;

    // Handle quoted command
    char* commandEnd = NULL;
    if (*commandStart == '\'' || *commandStart == '"') {
        char quote = *commandStart;
        commandStart++;
        commandEnd = strrchr(commandStart, quote);
        if (commandEnd == NULL) {
            printf("ERR: alias: Invalid format\n");
            free(newCom);
            return;
        }
        *commandEnd = '\0';
    }

    // Check if alias already exists and update if it does
    for (int i = 0; i < *aliasCount; i++) {
        if (strcmp(newCom, (*alias)[i].newCom) == 0) {
            free((*alias)[i].command);
            (*alias)[i].command = strdup(commandStart);
            free(newCom);
            return;
        }
    }

    // Reallocate memory to accommodate new alias
    map* temp = realloc(*alias, (*aliasCount + 1) * sizeof(map));
    if (temp == NULL) {
        perror("realloc");
        free(newCom);
        return;
    }
    *alias = temp;

    // Add new alias
    (*alias)[*aliasCount].newCom = newCom;
    (*alias)[*aliasCount].command = strdup(commandStart);
    if (strchr((*alias)[*aliasCount].command, '"') != NULL || strchr((*alias)[*aliasCount].command, '\'') != NULL) {
        (*total)++;
    }
    (*aliasCount)++;
    (*cmd)++;
}

void printAliases(map* alias, int aliasCount) {
    if (alias == NULL) return;

    for (int i = 0; i < aliasCount; i++) {
        printf("alias %s='%s'\n", alias[i].newCom, alias[i].command);
    }
}

void removeAlias(map** alias, int* aliasCount, char* input) {
    if (alias == NULL || input == NULL) return;

    char* aliasInput = input + 8; // Assuming input starts with "unalias "
    char* newCom = strdup(aliasInput);

    if (newCom == NULL) {
        printf("ERR: unalias: Memory allocation failed\n");
        return;
    }

    for (int i = 0; i < *aliasCount; i++) {
        if (strcmp(newCom, (*alias)[i].newCom) == 0) {
            free((*alias)[i].newCom);
            free((*alias)[i].command);

            // Shift remaining aliases down
            for (int j = i; j < *aliasCount - 1; j++) {
                (*alias)[j] = (*alias)[j + 1];
            }

            (*aliasCount)--;

            map* temp = realloc(*alias, (*aliasCount) * sizeof(map));
            if (temp != NULL || *aliasCount == 0) {
                *alias = temp;
            }

            free(newCom);
            return;
        }
    }

    printf("ERR: unalias: Alias not found\n");
    free(newCom);
}

void readScript(char* fileName, int* lines, map** alias, int* aliasCount, int* cmd, int* total) {
    // Check if the file has a .sh extension
    const char* ext = strrchr(fileName, '.');
    if (!ext || strcmp(ext, ".sh") != 0) {
        printf("ERR: ./%s: command not found\n",fileName);
        return;
    }

    FILE *script = fopen(fileName, "r");
    if (script == NULL) {
        perror("fopen");
        return;
    }

    char input[MAX_COMMAND_LENGTH];
    char **args;
    fgets(input, MAX_COMMAND_LENGTH, script);
    input[strcspn(input, "\n")] = '\0';
    if (strcmp(input, "#!/bin/bash") != 0) {
        printf("file cannot be compiled as bash\n");
        fclose(script);
        return;
    }
    (*lines)++;

    while (fgets(input, MAX_COMMAND_LENGTH, script) != NULL) {
        input[strcspn(input, "\n")] = '\0';
        args = splitSentenceIntoWords(input,total);
        (*lines)++;


        if (args == NULL || args[0] == NULL) {
            freeArguments(args);
            continue;
        }

        if (args[0][0]=='#'){
            continue;
        }

        if (strcmp(input, "alias") == 0) {
            printAliases(*alias, *aliasCount);
            (*cmd)++;
            (*lines)++;
            freeArguments(args);
            continue;
        } else if (strcmp(args[0], "alias") == 0) {
            addAlias(alias, aliasCount, input,total,cmd);
            freeArguments(args);
            (*cmd)++;
            continue;
        }

        if (strcmp(args[0], "unalias") == 0) {
            removeAlias(alias, aliasCount, input);
            freeArguments(args);
            (*cmd)++;
            continue;
        }

        for (int i = *aliasCount - 1; i >= 0; i--) {
            if (strcmp(args[0], (*alias)[i].newCom) == 0) {
                freeArguments(args);
                args = splitSentenceIntoWords((*alias)[i].command,total);
                if (strchr((*alias)[*aliasCount].command,'"')!=NULL||strchr((*alias)[*aliasCount].command,'\'')!=NULL){
                    (*total)--;
                }
                break;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            freeArguments(args);
            fclose(script);
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            execvp(args[0], args);
            fprintf(stderr,"%s: command not found\n",args[0]);
            freeArguments(args);
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            if (WEXITSTATUS(status) == 0) {
                (*cmd)++;
            } else {
                printf("ERR: command not found\n");
            }
        }

        freeArguments(args);
    }

    fclose(script);
}

void freeArguments(char** args) {
    if (args == NULL) return;

    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }

    free(args);
}

void freeAliases(map* alias, int aliasCount) {
    if (alias == NULL) return;

    for (int i = 0; i < aliasCount; i++) {
        free(alias[i].newCom);
        free(alias[i].command);
    }

    free(alias);
}

char** replaceAliasWithCommand(map* alias, int aliasCount, char** argv, int* total) {
    for (int i = aliasCount - 1; i >= 0; i--) {
        if (strcmp(argv[0], alias[i].newCom) == 0) {
            size_t newInLength = strlen(alias[i].command) + 1; // +1 for space
            for (int j = 1; argv[j] != NULL; j++) {
                newInLength += strlen(argv[j]) + 1; // +1 for space or null terminator
            }

            char* newIn = (char*)malloc(newInLength);
            if (newIn == NULL) {
                perror("malloc");
                freeArguments(argv);
                exit(EXIT_FAILURE);
            }

            strcpy(newIn, alias[i].command);
            strcat(newIn, " "); // Add a space after the alias command

            for (int j = 1; argv[j] != NULL; j++) {
                strcat(newIn, argv[j]);
                if (argv[j + 1] != NULL) {
                    strcat(newIn, " "); // Add a space between arguments
                }
            }
            freeArguments(argv);
            argv = splitSentenceIntoWords(newIn, total);

            free(newIn);

            break;
        }
    }
    return argv;
}

int exec_in_background(job** jobs, char* command, int* num_of_processes, int* total) {
    int status = 0;
    if (command[strlen(command) - 1] == '&') {
        command[strlen(command) - 1] = '\0';  // Remove the '&' at the end
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char** argv = splitSentenceIntoWords(command, total);
        if (execvp(argv[0], argv) == -1) {
            fprintf(stderr, "%s: command not found\n", argv[0]);
            freeArguments(argv);
            exit(EXIT_FAILURE);  // Exit the child process if exec fails
        }
        freeArguments(argv);
    } else {
        int exec_status;
        waitpid(pid, &exec_status, WNOHANG);  // Check if child process failed immediately

        if (WIFEXITED(exec_status) && WEXITSTATUS(exec_status) == EXIT_FAILURE) {
            status = -1;  // Indicate that execvp failed
        } else {
            job* temp = realloc(*jobs, (*num_of_processes + 1) * sizeof(job));
            if (temp == NULL) {
                perror("realloc");
                status = -1;
            } else {
                *jobs = temp;
                (*jobs)[*num_of_processes].pid = pid;
                (*jobs)[*num_of_processes].command = strdup(command);
                (*num_of_processes)++;
            }
        }
    }
    return status;
}

void check_if_end(job** jobs, int* num_of_processes,int* cmd) {
    if (jobs == NULL || *num_of_processes == 0) return;

    for (int i = 0; i < *num_of_processes; i++) {
        int status;
        pid_t result = waitpid((*jobs)[i].pid, &status, WNOHANG);
        if (result != 0) {
            delete_process(jobs, i, num_of_processes);
            (*cmd)++;
            i--;
        }
    }
}

void delete_process(job** jobs, int index, int* num_of_processes) {
    if (jobs == NULL || *jobs == NULL || index < 0|| index>=*num_of_processes) return;

    free((*jobs)[index].command);

    for (int i = index; i < *num_of_processes - 1; i++) {
        (*jobs)[i] = (*jobs)[i + 1];
    }

    (*num_of_processes)--;
    *jobs = realloc(*jobs, (*num_of_processes) * sizeof(job));
}

void print_all_jobs(job* jobs, int num_of_jobs) {
    if (jobs==NULL||num_of_jobs==0){
        return;
    }
    for (int i = 0; i < num_of_jobs; i++) {
        printf("[%d]               %s&\n", i+1,(jobs)[i].command);
    }
}

void freeJobs(job* jobs, int num_of_jobs) {
    if (jobs == NULL) return;

    for (int i = 0; i < num_of_jobs; i++) {
        free(jobs[i].command);
    }
    free(jobs);
}

int execute_command(char* cmd,int* total) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        char** args = splitSentenceIntoWords(cmd, total);
        if (args == NULL) {
            exit(EXIT_FAILURE);
        }
        execvp(args[0], args);
        freeArguments(args);
        // If execvp returns, it must have failed
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return 0;  // Command executed successfully
        } else {
            return -1;  // Command failed to execute
        }
    }
}

int* Count_AND_OR(char** args, int* commandCount) {
    if (args==NULL)return NULL;
    int j=0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "&&") == 0||strcmp(args[i], "||") == 0) {
            j++;
        }
    }
    *commandCount = j+1; // Set the total number of commands
    int* operations=(int*)malloc(j*sizeof (int*));
    j=0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "&&") == 0) {
            operations[j]=0;
            j++;
        } else if (strcmp(args[i], "||") == 0) {
            operations[j]=1;
            j++;
        }
    }

    // Make sure the operations array is correctly sized
    if (operations == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }

    return operations;
}

char** separate_input_AND_OR(char* input, int commandsCount) {
    char** commands = (char**)malloc(commandsCount * sizeof(char*));
    int s=0;
    int u=0;
    int* sites=(int*) malloc(commandsCount*sizeof (int*));
    while (input[s]!='\0'){
        if (input[s]=='&'||input[s]=='|'){
            sites[u]=s;
            s++;
            u++;
        }
        s++;
    }
    sites[u]=(int) strlen(input); // changed -1 to 0
    int k; // commands iterator
    int i=0;
    for (k=0; k<commandsCount; k++) {
        commands[k] = (char*)malloc((sites[k] - i + 1) * sizeof(char)); // allocate memory for each command string
    }
    k=0;
    i=0;
    s=0;
    while (k<commandsCount&&input[i]!='\0'){
        while (i<sites[k]){
            commands[k][s]=input[i];
            i++;
            s++;
        }
        commands[k][s]='\0';
        s=0;
        i+=2;
        while (input[i]==' '){
            i++;
        }
        k++;
    }

    free(sites);
    if (input[strlen(input)-1]=='&'){
        strcat(commands[commandsCount-1],"&");
    }
    for (int n=0; n<commandsCount; n++){
        if (commands[n][strlen(commands[n])-1]==' ') {
            commands[n][strlen(commands[n])-1]='\0';
            commands[n]=(char*) realloc(commands[n], (strlen(commands[n])-1)*sizeof (char*));
        }
    }
    return commands;
}

void manage_AND_OR(char** args, const int* operations, int cmdCount, int* status,int* total,int op,int* cmd,job** jobs,int* num_of_jobs) {

    if (args == NULL || operations == NULL) return;
    if (cmdCount == 0) {
        return; // Base case: no more commands to execute
    }

    if (args[0][strlen(args[0])-1]=='&'){
        int exe=exec_in_background(jobs,args[0],num_of_jobs,total);
        if((*cmd)!=0){
            (*cmd)--;
        }
        *status=0;
        if (exe!=0){
            (*cmd)--;
            *status=-1;
        }
    }

    else if (strcmp(args[0],"jobs")==0&&jobs!=NULL&&*jobs!=NULL){
        print_all_jobs(*jobs,*num_of_jobs);
        *status=0;
    }

    else {*status = execute_command(args[0],total);}
    char** temp = splitSentenceIntoWords(args[0], total); //to check the number of arguments if it more than the limit
    if (temp == NULL) return;
    freeArguments(temp);

    if (*status == -1) {
        if (op != -1) {
            char buffer[MAX_COMMAND_LENGTH];
            ssize_t size = sprintf(buffer, "%s: command not found\n", args[0]);
            write(op, buffer, size);
        }
        else printf ("%s: command not found\n",args[0]);
    }

    if (*status==0&&cmd!=NULL){
        (*cmd)++;
    }

    // Determine if we should proceed to the next command based on operations and status
    if (cmdCount > 1) {
        int proceed = 0;
        if ((operations[0] == 0 && *status == 0) || // AND: proceed if previous command succeeded
            (operations[0] == 1 && *status != 0) || // OR: proceed if previous command failed
            (*status == -1 && operations[0] == 1)) { // Proceed if command not found and next is OR
            proceed = 1;
        }

        if (proceed) {

            // Prepare new arguments and operations by excluding the#cmd:0|#alias:0|#script lines:0> echo 1 23 4 5 2> f.txt
            char** newArgs = (char**)malloc((cmdCount - 1) * sizeof(char*));
            int* newOperations = (int*)malloc((cmdCount - 1) * sizeof(int));
            if (newArgs == NULL || newOperations == NULL) {
                perror("Failed to allocate memory");
                exit(EXIT_FAILURE);
            }

            for (int i = 1; i < cmdCount; i++) {
                newArgs[i - 1] = strdup(args[i]);
                if (i < cmdCount - 1) {
                    newOperations[i - 1] = operations[i];
                }
            }
            // Recursively call manage_AND_OR with the new arrays
            manage_AND_OR(newArgs, newOperations, cmdCount - 1, status,total,op,cmd,jobs,num_of_jobs);

            free (newArgs[0]);
            free (newArgs);
            free (newOperations);

        }
    }
}

void ERR_to_file(char* input) {
    char command[MAX_COMMAND_LENGTH];
    char file_name[MAX_COMMAND_LENGTH];
    int i = 0, j = 0;

    // Parse command
    while (input[i] != '>' && input[i] != '\0') {
        command[i] = input[i];
        i++;
    }
    i--;
    command[i] = '\0';
    i++;

    if (input[i] == '\0') {
        fprintf(stderr, "Invalid input: Missing '>'\n");
        return;
    }

    // Skip '>' and any spaces after it
    i++;
    while (input[i] == ' ') {
        i++;
    }

    // Parse filename
    while (input[i] != '\0') {
        file_name[j] = input[i];
        i++;
        j++;
    }
    file_name[j] = '\0';

    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        close(fd[0]);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);

        char** args = splitSentenceIntoWords(command, 0);
        execvp(args[0], args);

        fprintf(stderr,"%s: command not found\n",args[0]);
        freeArguments(args);
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(fd[1]);

        int op = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (op == -1) {
            perror("open");
            close(fd[0]);
            exit(EXIT_FAILURE);
        }

        char buffer[MAX_COMMAND_LENGTH];
        ssize_t size;
        while ((size = read(fd[0], buffer, sizeof(buffer))) > 0) {
            write(op, buffer, size);
        }

        close(fd[0]);
        close(op);
        wait(NULL);
    }
}

void ERR_and_or_to_file(const char* input) {
    char command[MAX_COMMAND_LENGTH];
    char file_name[MAX_COMMAND_LENGTH];
    int i = 1, j = 0;

    const char* redir_pos = strstr(input, "2>");
    if (redir_pos == NULL) {
        fprintf(stderr, "Invalid input: Missing '2>'\n");
        return;
    }

    int n=0;
    while (&input[i] < redir_pos && input[i] != '\0' && input[i] != ')') {
        command[n] = input[i];
        i++;
        n++;
    }
    command[n] = '\0';

    redir_pos += 2;
    while (*redir_pos == ' ') {
        redir_pos++;
    }

    // Copy the filename part
    while (*redir_pos != '\0') {
        file_name[j++] = *redir_pos++;
    }
    file_name[j] = '\0';

    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int op = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (op == -1) {
        perror("open");
        close(fd[0]);
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        close(fd[0]);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);

        int cmdCounter=0;
        int status=0;
        char** argv= splitSentenceIntoWords(command,0);
        int* operations=Count_AND_OR(argv,&cmdCounter);
        char** commands= separate_input_AND_OR(command,cmdCounter);
        manage_AND_OR(commands,operations,cmdCounter,&status,0,op,0,NULL,0);
        freeArguments(argv);
        freeArguments(commands);
        free (operations);
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(fd[1]);



        char buffer[MAX_COMMAND_LENGTH];
        ssize_t size;
        while ((size = read(fd[0], buffer, sizeof(buffer))) > 0) {
            write(op, buffer, size);
        }

        close(fd[0]);
        close(op);
        wait(NULL);
    }
}

int main() {
    map* alias = NULL;
    int cmdCount = 0, aliasCount = 0, scriptLines = 0;
    char prompt[50];
    int total = 0;
    int num_of_processes = 0;
    job* jobs = NULL;

    updatePrompt(0, 0, 0, prompt);

    while (1) {
        char input[MAX_COMMAND_LENGTH];
        char **argv;

        updatePrompt(cmdCount, aliasCount, scriptLines, prompt);
        printf("%s", prompt);
        fgets(input, MAX_COMMAND_LENGTH, stdin);

        input[strcspn(input, "\n")] = '\0';

        if ((strstr (input,"2>")!=NULL)&&(strstr(input,"&&")==NULL&&strstr(input,"||")==NULL)){
            ERR_to_file(input);
            continue;
        }

        argv = splitSentenceIntoWords(input,&total);

        check_if_end(&jobs, &num_of_processes,&cmdCount);

        if (argv == NULL) {
            freeArguments(argv);
            continue;
        }

        if (argv[0] == NULL) {
            freeArguments(argv);
            continue;
        }

        if (strcmp(input, "exit_shell") == 0) {
            printf("%d\n", total);
            freeArguments(argv);
            break;
        }

        if (strcmp(input, "alias") == 0) {
            cmdCount++;
            printAliases(alias, aliasCount);
            freeArguments(argv);
            continue;
        }

        if (strcmp(argv[0], "alias") == 0 && argv[1] != NULL) {
            addAlias(&alias, &aliasCount, input, &total,&cmdCount);
            freeArguments(argv);
            continue;
        }

        argv = replaceAliasWithCommand(alias, aliasCount, argv, &total);

        if (strcmp(argv[0], "unalias") == 0) {
            removeAlias(&alias, &aliasCount, input);
            cmdCount++;
            freeArguments(argv);
            continue;
        }

        if (strcmp(argv[0], "source") == 0) {
            readScript(argv[1], &scriptLines, &alias, &aliasCount, &cmdCount,&total);
            freeArguments(argv);
            continue;
        }

        if (strcmp(argv[0], "jobs") == 0) {
            print_all_jobs(jobs, num_of_processes);
            cmdCount++;
            freeArguments(argv);
            continue;
        }

        if ((strstr(input,"&&")!=NULL||strstr(input,"||")!=NULL)&&strstr (input,"2>")==NULL){
            int cmdCounter=0;
            int status=0;
            int* operations=Count_AND_OR(argv,&cmdCounter);
            char** commands= separate_input_AND_OR(input,cmdCounter);
            manage_AND_OR(commands,operations,cmdCounter,&status,&total,-1,&cmdCount,&jobs,&num_of_processes);
            free(operations);
            freeArguments(argv);
            continue;
        }

        if ((strstr(input, "2>") != NULL) && (strstr(input, "&&") != NULL || strstr(input, "||") != NULL)) {
            ERR_and_or_to_file(input);
            freeArguments(argv);
            continue;
        }

        if (input[strlen(input)-1]=='&') {
            exec_in_background(&jobs, input, &num_of_processes, &total);
            freeArguments(argv);
            continue;
        }

        int status=execute_command(input,0);

        if (status!=0){
            printf ("%s: command not found\n",input);
        }
        if (status==0){
            cmdCount++;
        }
        freeArguments(argv);
    }

    freeJobs(jobs,num_of_processes);

    freeAliases(alias, aliasCount);

    return 0;
}
