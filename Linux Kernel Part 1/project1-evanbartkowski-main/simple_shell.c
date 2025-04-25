/*
project: 01
author:  Evan Bartkowski
email:  e168@umbc.edu
student id: gb70971
description: a simple linux shell designed to perform basic linux commands
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include "utils.h"

// FOR PART 3 HISTORY
#define MAX_HISTORY 10
#define HISTORY_FILE ".421sh"  //this is the secret folder
char *history[MAX_HISTORY];
int history_count = 0;
void save_command_to_history(const char *command);
void load_history(void);
void free_history(void);

// Part two functions for proc file
void read_cpuinfo(void);
void read_loadavg(void);
void read_process_status(pid_t pid);
void read_process_environ(pid_t pid);
void read_process_sched(pid_t pid);

// DEFINE THE FUNCTION PROTOTYPES Part one
void user_prompt_loop(void);
char *get_user_command(void);
char **parse_command(char *input);
void execute_command(char **args);

////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv){
    if (argc > 1) {
        fprintf(stderr, "Error: 421shell does not support command-line arguments\n");
        return EXIT_FAILURE; }

    user_prompt_loop(); // THIS STARTS SHELL LOOOP
    free_history();
    return EXIT_SUCCESS;}
////////////////////////////////////////////////////////////////////////////
void user_prompt_loop(void) {
    char *input;
    char **args;

    while (1) {
        // Display prompt everytime for input from user
        printf("421shell> ");

        // Get user 
        input = get_user_command();

        // Parses
        args = parse_command(input);

        // Check if arg is null
        if (args[0] == NULL) {
            free(input);
            free(args);
            continue;}

   // Check if the first command is  "exit"
   if (strcmp(args[0], "exit") == 0) {
    	int exit_code = 0; 

    // Check if an exit code is provided
   	 if (args[1] != NULL) {
        	exit_code = atoi(args[1]);}

    	free(input);
    	free(args);
    	free_history();
    	// Exit with the provided exit code
    	exit(exit_code);} 

        // Check if commands  "history"
        if (strcmp(args[0], "history") == 0) {
            print_history();
            free(input);
            free(args);
            continue;}

        // Saves history command
        save_command_to_history(input);


        // Check if the command is "/proc"
	// Make sure you use cat for these commands
        if (strncmp(args[0], "/proc", 5) == 0) {
            char *pid_str = strrchr(args[0], '/'); // Find last slash
            if (pid_str && pid_str[1] != '\0') {
                pid_t pid = atoi(pid_str + 1); // Converts PID
                if (strcmp(pid_str + 1, "cpuinfo") == 0) { 
                    read_cpuinfo(); //cpuinfo
                } else if (strcmp(pid_str + 1, "loadavg") == 0) {
                    read_loadavg(); //load avg command
                } else if (strcmp(pid_str + 1, "status") == 0) {
                    read_process_status(pid); //status command
                } else if (strcmp(pid_str + 1, "environ") == 0) {
                    read_process_environ(pid); //environ command
                } else if (strcmp(pid_str + 1, "sched") == 0) {
                    read_process_sched(pid);} //schedule command
		else {printf("Unknown /proc command\n");}}
		else {printf("Invalid command format\n");}
            free(input);
            free(args);
            continue;}

        execute_command(args);

        // Free memory and dynamic allocation
        free(input);
        free(args);}}
////////////////////////////////////////////////////////////////////////////
char *get_user_command(void){
    char *input = NULL;
    size_t buffetsize = 0;

    // getline cuz were not allowed to uses scan
    if (getline(&input, &buffetsize, stdin) == -1) {
        perror("getline failed");
        exit(EXIT_FAILURE);}

    // Remove the newline charact
    input[strcspn(input, "\n")] = '\0';

    return input;}
////////////////////////////////////////////////////////////////////////////
char **parse_command(char *input){
    char **args = malloc(64 * sizeof(char *)); // Allocate space dynamically
    char *token;
    int positionn = 0;

    token = strtok(input, " ");
    while (token != NULL) {
        args[positionn++] = token;
        token = strtok(NULL, " ");}
    args[positionn] = NULL;

    return args;}
////////////////////////////////////////////////////////////////////////////
void execute_command(char **args){
    pid_t pid, wpid;
    int status;

    // Fork the child
    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("execvp failed");}
        exit(EXIT_FAILURE); // Exit if execvp failores
    } else if (pid < 0) {
        perror("fork failed");}
	else{
        // waits for the child to complete
        do{
            wpid = waitpid(pid, &status, WUNTRACED);}
	while (!WIFEXITED(status) && !WIFSIGNALED(status));}}
////////////////////////////////////////////////////////////////////////////
void read_cpuinfo(void){
    FILE *file = fopen("/proc/cpuinfo", "r");
    if (file == NULL){
        perror("Failed  to open /proc/cpuinfo");
        return;}

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);}
    fclose(file);}
////////////////////////////////////////////////////////////////////////////
void read_loadavg(void){
    FILE *file = fopen("/proc/loadavg", "r");
    if (file == NULL){
        perror("Failed to open /proc/loadavg");
        return;}

    char line[256];
    while (fgets(line, sizeof(line), file)){
        printf("%s", line);}
    fclose(file);}
////////////////////////////////////////////////////////////////////////////
void read_process_status(pid_t pid){
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/status", pid);
    FILE *file = fopen(filepath, "r");
    if (file == NULL){
        perror("Failed to open /proc/[pid]/status");
        return;}

    char line[256];
    while (fgets(line, sizeof(line), file)){
        printf("%s", line);}
    fclose(file);}
//////////////////////////////////////////////////////////////////////////// 
void read_process_environ(pid_t pid){
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/environ", pid);
    FILE *file = fopen(filepath, "r");
    if (file == NULL){
        perror("Failed to open /proc/[pid]/environ");
        return;}

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        //separated null characters var
        char *env = strtok(line, "\0");
        while (env) {
            printf("%s\n", env);
            env = strtok(NULL, "\0");}}
    fclose(file);}
//////////////////////////////////////////////////////////////////////////// 
void read_process_sched(pid_t pid){
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/sched", pid);
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Failed to open /proc/[pid]/sched ");
        return;}

    char line[256];
    while (fgets(line, sizeof(line), file)){
        printf("%s", line);}
    fclose(file);}
////////////////////////////////////////////////////////////////////////////
void save_command_to_history(const char *command) {
    // Stores command in an array which will be added to 421 secret folder
    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(command);}
	else {
        free(history[0]); // RemoveS oldest
        for (int i = 1; i < MAX_HISTORY; i++) {
            history[i - 1] = history[i];}
        history[MAX_HISTORY - 1] = strdup(command);}

    //SENDS COMMAND TO 421
    FILE *file = fopen(HISTORY_FILE, "a");
    if (file){
        fprintf(file, "%s\n", command);
        fclose(file);}
	else{
        perror("Failure has occured tryig to open history file");}}


void print_history(void) {
    int start = history_count > MAX_HISTORY ? history_count - MAX_HISTORY : 0;
    for (int i = start; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);}}
/////////////////////////////////////////////////////////////////////////////
void free_history(void){
    for (int i = 0; i < history_count; i++) {
        free(history[i]);}}
/////////////////////////////////////////////////////////////////////////////
