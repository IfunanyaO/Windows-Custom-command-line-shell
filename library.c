#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifdef _WIN32
#else
#endif

#define MAX_CMD_LEN 1024
#define MAX_HISTORY 100
#define MAX_JOBS 100
#define CHUNK_SIZE 1024 * 1024

typedef struct {
    int id;
    char command[MAX_CMD_LEN];
    PROCESS_INFORMATION pi;
    int is_background;
} Job;

typedef struct {
    HANDLE hSourceFile;
    HANDLE hDestFile;
    DWORD64 offset;
    DWORD size;
} ThreadData;

Job jobs[MAX_JOBS];
int job_count = 0;
char history[MAX_HISTORY][MAX_CMD_LEN];
int history_count = 0;

const char *command_list[] = {"cd", "pwd", "exit", "jobs", "echo","dir","fg", "kill", "history", "ls", "clear", NULL};

// Function declarations
void execute_command(const char *cmd, int is_background);
const char* enable_auto_completion(const char *input);
void change_directory(const char *path);
void print_working_directory();
void list_jobs();
void bring_to_foreground(int job_id);
void kill_job(int job_id);
void remove_job(int job_id);
void add_to_history(const char *cmd);
void print_history();
void trim_whitespace(char *str);
void redirect_output(const char *cmd, const char *file);
void redirect_input(const char *cmd, const char *file);
void execute_pipeline(char *cmd1, char *cmd2);
void enable_virtual_terminal_processing();
void load_config(const char* config_file);
void my_find(const char* keyword);
void copy_file_multithreaded(const char *source, const char *destination);

int main() {


    // This is for file support config
    char* setting1 = getenv("setting1");
    if (setting1) {
        printf("Setting1: %s\n", setting1);
    }
    char cmd[MAX_CMD_LEN];

    while (1) {
        printf("myshell> ");
        if (!fgets(cmd, MAX_CMD_LEN, stdin)) continue;

        cmd[strcspn(cmd, "\n")] = 0;  // code to remove the newline character
        if (strlen(cmd) == 0) continue;

        int is_background = 0;
        char *ampersand = strrchr(cmd, '&');
        if (ampersand && *(ampersand + 1) == '\0') {
            is_background = 1;
            *ampersand = '\0'; // Remove '&' from command after adding jobs
            //Code to remove leading spaces
            while (strlen(cmd) > 0 && isspace(cmd[strlen(cmd) - 1])) {
                cmd[strlen(cmd) - 1] = '\0';
            }
        }

        add_to_history(cmd);

        // Auto-completion code: This is to suggest commands based on the input the user entered
        if (strncmp(cmd, "suggest ", 8) == 0) {
            const char *suggested = enable_auto_completion(cmd + 8);
            if (suggested) {
                strcpy(cmd, suggested);  // here, i am replacing the original cmd with the suggested command so it can run the command
            } else {
                continue;  // If there is no match, we will continue by skipping to the next loop
            }
        }

        // If statement to process or run all the command
        if (strncmp(cmd, "cd ", 3) == 0)// command code to switch directory
            change_directory(cmd + 3);
        else if (strcmp(cmd, "pwd") == 0)// command code to print the current directory
            print_working_directory();
        else if (strcmp(cmd, "exit") == 0)// command code to exit the terminal
            exit(0);
        else if (strcmp(cmd, "jobs") == 0)// command to print and list all the jobs
            list_jobs();
        else if (strcmp(cmd, "history") == 0)// command to print o past inputed commands
            print_history();
        else if (strcmp(cmd, "clear") == 0)// command to clear all previous dialog
            system("cls");
        else if (strcmp(cmd, "dir") == 0) //command to show all files in directory
            system("dir");
        else if (strncmp(cmd, "load_config ", 12) == 0) { // command to test the file support config and set the envionment
            char *filename = cmd + 12;
            trim_whitespace(filename);// remove any beginning or leading spaces
            load_config(filename);
        } else if (strncmp(cmd, "getenv ", 7) == 0) { // code to get env
            char *key = cmd + 7;
            trim_whitespace(key);// remove any beginning or leading spaces
            char *val = getenv(key);
            if (val) {
                printf("%s=%s\n", key, val); // setenv if key then set it to the value
            } else {
                printf("Environment variable '%s' not found.\n", key); // handle error if environment not found
            }
        }
        else if (strncmp(cmd, "fg ", 3) == 0) // command to bring job to foregrounf
            bring_to_foreground(atoi(cmd + 3));
        else if (strncmp(cmd, "kill ", 5) == 0)// command to kil or terminate a job
            kill_job(atoi(cmd + 5));
        else if (strncmp(cmd, "copy ", 5) == 0) { // command to copy
            char *source = strtok(cmd + 5, " ");
            char *dest = strtok(NULL, " ");
            if (source && dest) {
                copy_file_multithreaded(source, dest);
            } else {
                printf("Usage: copy <source> <destination>\n");// this is a code to copy the content of what is in the souce file to the destination file
            }
        }
        else if (strchr(cmd, '|')) {// if statement for piplining
            char *left_cmd = strtok(cmd, "|");
            char *right_cmd = strtok(NULL, "|");

            if (!left_cmd || !right_cmd) {
                printf("Error: Invalid pipeline syntax\n"); // handle error or mis sntax form in pipelning
                continue;
            }

            trim_whitespace(left_cmd);// remove any beginning or leading spaces
            trim_whitespace(right_cmd);// remove any beginning or leading spaces

            // code to handle myfind command in the terminal
            if (strncmp(right_cmd, "myfind", 6) == 0) {// code to find a word in a sentence
                char *keyword = right_cmd + 6;
                trim_whitespace(keyword);// remove any beginning or leading spaces

                if (strlen(keyword) == 0) {
                    printf("Error: myfind requires a keyword\n");// if satement to handle errors
                    continue;
                }

                // Run left command and pipe its output into myfind
                FILE *pipe = _popen(left_cmd, "r");
                if (!pipe) {// if to handle pipe commands
                    printf("Error: Failed to execute command: %s\n", left_cmd);// error message if not pip or missyntax
                    continue;
                }

                // cod i used to enable ANSI color on Windows but failed
                HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                DWORD mode = 0;
                GetConsoleMode(hOut, &mode);
                SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

                char line[MAX_CMD_LEN];
                while (fgets(line, sizeof(line), pipe)) {
                    char *pos = strstr(line, keyword);
                    if (pos) {
                        *pos = '\0';
                        printf("%s", line);
                        printf("\x1b[32m%s\x1b[0m", keyword);
                        printf("%s", pos + strlen(keyword));
                    }
                }

                _pclose(pipe);
            } else {
                // Handle regular pipeline with external commands
                execute_pipeline(left_cmd, right_cmd);
            }


        } else if (strchr(cmd, '>')) { // If statement to handle output redirection
            char *file = strchr(cmd, '>') + 1;
            *strchr(cmd, '>') = '\0';
            redirect_output(cmd, file);
        }else if (strchr(cmd, '<')) {// if statement to handle input redirection
            char *redirect = strchr(cmd, '<');
            if (redirect) {
                *redirect = '\0';  // To tell the terminal to terminate the command before '<'
                char *file = redirect + 1;

                trim_whitespace(file);  // Code to clean up spaces as usual or leading spaces
                trim_whitespace(cmd);   // Clean command part spaces if an acidentally

                printf("Trying to open: '%s'\n", file);  // Debug print

                if (strlen(cmd) == 0) {
                    // If here is no command specified or requestd then the terminal should treat file as a script or batch of commands instad
                    FILE *fp = fopen(file, "r");// opening file
                    if (fp) {
                        char line[MAX_CMD_LEN];
                        while (fgets(line, sizeof(line), fp)) {
                            line[strcspn(line, "\n")] = 0;
                            if (strlen(line) > 0) {// if length of string is 0 then execute the command
                                execute_command(line, 0);
                            }
                        }
                        fclose(fp);// close the file after used
                    } else {
                        printf("Error opening file for input redirection: %lu\n", GetLastError());// print statement to handle input redirection failure
                    }
                } else {
                    // A command t handle sorting if request by the user
                    redirect_input(cmd, file);
                }
            } else {
                printf("Error: No '<' symbol found\n");// if user forgets to put < print this error message
            }
        }

        else {
            // Execute external command if not any of the option above
            execute_command(cmd, is_background);
        }
    }

    return 0;
}
// trim write spacs to handle spaces
void trim_whitespace(char *str) {

    while (isspace((unsigned char)*str)) str++;

    // handle all leading spaces
    if (*str == 0) return;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0';
}

// code to handle file support config
void load_config(const char* config_file) {
    FILE* file = fopen(config_file, "r");// firs pen the file
    if (!file) {
        perror("Failed to open configuration file");// if file cannot be opened print error message
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // code to skip all the empty lines or comments
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        // code to split the key-value pair
        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n");

        if (key && value) {
            trim_whitespace(key);// remove spaces for the key
            trim_whitespace(value);// remove spaces for values too
#ifdef _WIN32
            // Correct Windows way
            char full_setting[256];
            snprintf(full_setting, sizeof(full_setting), "%s=%s", key, value);
            _putenv(full_setting);// putting the environment variable
#else
            setenv(key, value, 1); //setting the environment variables
#endif
            printf("Loaded %s=%s\n", key, value);// code to print the key = value rsult
        }
    }

    fclose(file);// close file after use
}

// code for the execute command function
void execute_command(const char *cmd, int is_background) {
    // Check if the command is 'echo' and if it is handle it following instrucions below
    if (strncmp(cmd, "echo ", 5) == 0) {
        // here is the code telling the terminal to skip the word "echo " and print the rest of the command folowing it
        printf("%s\n", cmd + 5);// printing
        return;
    }
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi;
    si.cb = sizeof(si);

    char wrapped_cmd[MAX_CMD_LEN];
    snprintf(wrapped_cmd, sizeof(wrapped_cmd), "cmd.exe /c %s", cmd);
// if to handle not creating process
    if (!CreateProcess(NULL, wrapped_cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        printf("Error executing command: %lu\n", GetLastError());// eror message to handle f a proces is not created
    } else {
        if (is_background) {// checking if it is a background job being added
            if (job_count < MAX_JOBS) {// if the job count is not bigger than the maximum job count
                jobs[job_count].id = job_count + 1;// showing the job with the id
                strcpy(jobs[job_count].command, cmd);
                jobs[job_count].pi = pi;
                jobs[job_count].is_background = 1;
                job_count++;// add the job to the list

                printf("Started background job [%d] (PID: %lu)\n", job_count, pi.dwProcessId);// prnt statement to let the use know that the job is runing
            } else {
                printf("Job limit reached.\n");// if the number of jos exceeds maximum count then print this message
                TerminateProcess(pi.hProcess, 0);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        } else {
            WaitForSingleObject(pi.hProcess, INFINITE);//if not wait for the process to finish
            CloseHandle(pi.hProcess);// close all handles for process
            CloseHandle(pi.hThread);// close all handles for thread
        }
    }
}

// function to handle auto-completion
const char* enable_auto_completion(const char *input) {
    static char match[MAX_CMD_LEN];
    int match_count = 0;// match count initialize to zero

    for (int i = 0; command_list[i] != NULL; i++) {// looping through the list of commands
        if (strncmp(command_list[i], input, strlen(input)) == 0) {
            if (++match_count == 1) {// if there is a match
                strcpy(match, command_list[i]);  // then save the first match
            } else {
                // In a situation here we have multiple matches then it should list all the possible suggestions and return nothing
                printf("Suggestions for '%s':\n", input);
                for (int j = 0; command_list[j] != NULL; j++) {
                    if (strncmp(command_list[j], input, strlen(input)) == 0) {
                        printf("  %s\n", command_list[j]);
                    }
                }
                return NULL;
            }
        }
    }

    if (match_count == 1) {
        printf("Auto-executing '%s'\n", match); // after suggesting it should execute the command
        return match;
    }

    printf("No matching commands found.\n");// if ther iso matchor suggestion then print the error message
    return NULL;
}

// code to handle directory
void change_directory(const char *path) {
    if (!SetCurrentDirectory(path)) {
        printf("Error changing directory: %lu\n", GetLastError());// print error message if directory was not successfully changed.
    }
}

// code to print the current working directory
void print_working_directory() {
    char buffer[MAX_PATH];
    if (GetCurrentDirectory(MAX_PATH, buffer)) {// if statement to get the current working directories
        printf("Current directory: %s\n", buffer);// printing the current directory
    } else {
        printf("Error getting current directory: %lu\n", GetLastError());// error to handle if getting the current directory was unsuccessful
    }
}

// code to handle listing job functions
void list_jobs() {
    for (int i = 0; i < job_count; i++) {// for loop to loop through all the jobs added
        printf("[%d] %s (PID: %lu)\n", jobs[i].id, jobs[i].command, jobs[i].pi.dwProcessId);// printing all the jobs added
    }
}
// code to handle bringing jobs to foreground
void bring_to_foreground(int job_id) {
    for (int i = 0; i < job_count; i++) { // loping through al jobs added
        if (jobs[i].id == job_id) {// checking if the job is equal to the id
            DWORD exit_code;
            WaitForSingleObject(jobs[i].pi.hProcess, INFINITE);// waiting until a process is completed
            GetExitCodeProcess(jobs[i].pi.hProcess, &exit_code);
            printf("Job [%d] terminated with exit code %lu\n", job_id, exit_code);// print statement once job is completed
            remove_job(i);// remove job once it is complete or done running
            return;
        }
    }
    printf("Job not found\n");// if there is no job that match or it have been previously terminated and was re run again print this error message
}

// code function to kill or terminate a job
void kill_job(int job_id) {
    for (int i = 0; i < job_count; i++) { // loping through al jobs added
        if (jobs[i].id == job_id) { // checking if the job is equal to the id
            TerminateProcess(jobs[i].pi.hProcess, 0);// kill the job
            printf("Job [%d] has been killed\n", job_id);// print statement letting the user know thejob as ben kiled
            remove_job(i);// remove the killed job
            return;
        }
    }
    printf("Job not found\n");//f there is no job that match or it have been previously terminated and was re run again print this error message
}

// code function to remove killed jobs or foreground jobs
void remove_job(int job_id) {
    for (int i = job_id; i < job_count - 1; i++) {// removing all kiled job or forground job from th list of jobs
        jobs[i] = jobs[i + 1];
    }
    job_count--; // remove the job
}

// function to save all commands and add them to history
void add_to_history(const char *cmd) {
    if (history_count < MAX_HISTORY) {// if statement to check if history count is smaller than the max history
        strcpy(history[history_count], cmd);//copy and save commands
        history_count++;// ad to the history
    } else {// if it is not smaller follow the instructions
        for (int i = 0; i < MAX_HISTORY - 1; i++) {//loop through all command history
            strcpy(history[i], history[i + 1]);
        }
        strcpy(history[MAX_HISTORY - 1], cmd);
    }
}

// code function to print command history inputed by the user
void print_history() {
    if (history_count == 0) {// if no command have been typed
        printf("No history available.\n"); //print this message to let users no there is no command history
        return;
    }

    for (int i = 0; i < history_count; i++) {// looping through all the commands inputed by the user
        printf("%d: %s\n", i + 1, history[i]);  // then printing each of all the command inputed by the user with its index
    }
}
//code to enable terminal processing virtually.
void enable_virtual_terminal_processing() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;// cod to mke sure it works

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

//code function to handle output redirection
void redirect_output(const char *cmd, const char *file) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    HANDLE hFile;

    // here i am setting up the security attributes struct
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    // CODE TO open the output file
    hFile = CreateFileA(file, GENERIC_WRITE, FILE_SHARE_WRITE, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error opening file for redirection: %lu\n", GetLastError());//error message to let users know it did not work
        return;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hFile;
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

    ZeroMemory(&pi, sizeof(pi));

    // Make a writable copy of the command
    //char cmd_copy[MAX_CMD_LEN];
    //strncpy(cmd_copy, cmd, MAX_CMD_LEN);
    char wrapped_cmd[MAX_CMD_LEN];
    snprintf(wrapped_cmd, MAX_CMD_LEN, "cmd.exe /c %s", cmd);

    if (!CreateProcessA(
        NULL,
        wrapped_cmd,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    )) {//if is not create process
        printf("Error executing command with redirection: %lu\n", GetLastError());// print his error to let the user know it did no work
        CloseHandle(hFile);
        return;
    }

    // Wait for all the process to finish, close all the handles used and clean up
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hFile);
}

//code function to handle redirect output
void redirect_input(const char *cmd, const char *file) {
    FILE *fp = fopen(file, "r"); // open file
    if (fp) {// if file successfully opens follow the commands below
        char line[MAX_CMD_LEN];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            execute_command(line, 0);
        }
        fclose(fp);// close fle
    } else {
        printf("Error opening file for input redirection\n");
    }
}


DWORD WINAPI CopyChunk(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    BYTE* buffer = (BYTE*)malloc(data->size);
    if (!buffer) {
        printf("Memory allocation failed\n");
        return 1;
    }

    DWORD bytesRead;
    SetFilePointer(data->hSourceFile, (LONG)data->offset, NULL, FILE_BEGIN);
    if (!ReadFile(data->hSourceFile, buffer, data->size, &bytesRead, NULL)) {
        printf("ReadFile failed with error %lu\n", GetLastError());
        free(buffer);
        return 1;
    }

    DWORD bytesWritten;
    SetFilePointer(data->hDestFile, (LONG)data->offset, NULL, FILE_BEGIN);
    if (!WriteFile(data->hDestFile, buffer, bytesRead, &bytesWritten, NULL)) {
        printf("WriteFile failed with error %lu\n", GetLastError());
        free(buffer);
        return 1;
    }

    free(buffer);
    return 0;
}

void copy_file_multithreaded(const char* source, const char* destination) {
    HANDLE hSourceFile = CreateFile(source, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSourceFile == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed with error %lu\n", GetLastError());
        return;
    }

    HANDLE hDestFile = CreateFile(destination, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDestFile == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed with error %lu\n", GetLastError());
        CloseHandle(hSourceFile);
        return;
    }

    DWORD64 fileSize = GetFileSize(hSourceFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        printf("GetFileSize failed with error %lu\n", GetLastError());
        CloseHandle(hSourceFile);
        CloseHandle(hDestFile);
        return;
    }

    DWORD numChunks = (DWORD)(fileSize / CHUNK_SIZE);
    if (fileSize % CHUNK_SIZE != 0) {
        numChunks++;
    }

    HANDLE* hThreads = (HANDLE*)malloc(numChunks * sizeof(HANDLE));
    ThreadData* threadData = (ThreadData*)malloc(numChunks * sizeof(ThreadData));

    for (DWORD i = 0; i < numChunks; i++) {
        threadData[i].hSourceFile = hSourceFile;
        threadData[i].hDestFile = hDestFile;
        threadData[i].offset = i * CHUNK_SIZE;
        threadData[i].size = (i == numChunks - 1) ? (DWORD)(fileSize - i * CHUNK_SIZE) : CHUNK_SIZE;

        hThreads[i] = CreateThread(NULL, 0, CopyChunk, &threadData[i], 0, NULL);
        if (hThreads[i] == NULL) {
            printf("CreateThread failed with error %lu\n", GetLastError());// error message if thred was not successfully created
            break;
        }
    }

    WaitForMultipleObjects(numChunks, hThreads, TRUE, INFINITE);

    for (DWORD i = 0; i < numChunks; i++) { // looping through all the chunks of words
        CloseHandle(hThreads[i]);// close all threads
    }

    free(hThreads);// free threads
    free(threadData);// also free thread data
    CloseHandle(hSourceFile); // close all handle source file
    CloseHandle(hDestFile);// as wl as close all handle destination files
}

void my_find(const char* keyword) {
    char line[MAX_CMD_LEN];

    // Thi code is to enable ANSI color support in my terminal to highlight but end up not working
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    while (fgets(line, sizeof(line), stdin)) {// loping through the line
        if (strstr(line, keyword)) {// if statement to check  line sentence with keyword to know if theis a match
            char* match = strstr(line, keyword);// If there is a match between the sentene in the line and the keywod
            *match = '\0';
            printf("%s", line);                      // print the line sentence
            printf("\x1b[32m%s\x1b[0m", keyword);    // Match the keyword found in green then
            printf("%s", match + strlen(keyword));   // print it along with the green highlighted match
        }
    }
}
// code function to handle pipe
void execute_pipeline(char *cmd1, char *cmd2) {
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
        fprintf(stderr, "Failed to create pipe: %lu\n", GetLastError());// error message if pipe was not successfully created
        return;
    }

    // First Command: cmd1

    STARTUPINFO si1;
    PROCESS_INFORMATION pi1;
    ZeroMemory(&si1, sizeof(STARTUPINFO));
    si1.cb = sizeof(STARTUPINFO);
    si1.hStdOutput = hWritePipe;  // Write to the end of pipe
    si1.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si1.dwFlags |= STARTF_USESTDHANDLES;

    char wrapped_cmd1[MAX_CMD_LEN];
    snprintf(wrapped_cmd1, sizeof(wrapped_cmd1), "cmd.exe /c \"%s\"", cmd1);

    if (!CreateProcess(NULL, wrapped_cmd1, NULL, NULL, TRUE, 0, NULL, NULL, &si1, &pi1)) {
        fprintf(stderr, "Failed to create process for command 1: %lu\n", GetLastError()); // error message if pipe was not successfully created
        CloseHandle(hWritePipe);// close handle as usual
        CloseHandle(hReadPipe);// close handle a usual
        return;
    }

    CloseHandle(hWritePipe); //code to show writing is completed or done.


    // do the same thing you dd for command 1 to the second Command which is cmd2

    STARTUPINFO si2;
    PROCESS_INFORMATION pi2;
    ZeroMemory(&si2, sizeof(STARTUPINFO));
    si2.cb = sizeof(STARTUPINFO);
    si2.hStdInput = hReadPipe;  // Read to the end of pipe
    si2.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si2.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si2.dwFlags |= STARTF_USESTDHANDLES;

    char wrapped_cmd2[MAX_CMD_LEN];
    snprintf(wrapped_cmd2, sizeof(wrapped_cmd2), "cmd.exe /c \"%s\"", cmd2);
// code to crate proceses
    if (!CreateProcess(NULL, wrapped_cmd2, NULL, NULL, TRUE, 0, NULL, NULL, &si2, &pi2)) {
        fprintf(stderr, "Failed to create process for command 2: %lu\n", GetLastError());// error message if pipe was not successfully created
        CloseHandle(hReadPipe);
        return;
    }

    // Closing the read handle because we no longer need it or use it again
    CloseHandle(hReadPipe);

    // Waiting for both processes to finish
    WaitForSingleObject(pi1.hProcess, INFINITE);
    WaitForSingleObject(pi2.hProcess, INFINITE);

    // Closing all handles used
    CloseHandle(pi1.hProcess);
    CloseHandle(pi1.hThread);
    CloseHandle(pi2.hProcess);
    CloseHandle(pi2.hThread);
}
