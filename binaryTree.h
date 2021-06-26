#ifndef TREE_LOGIC_H
#define TREE_LOGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#define SLEEPTIME 1000000 // w mikrosekundach
#define NUMBER_OF_PROCESSES 14 //Ilosc procesow, ktore zostana utworzone i zarzadzane przez drzewo
#define COMMUNICATION_ARRAY_SIZE 10 //Rozmair tablicy ktora jest generowana
#define NUMBER_OF_ITERATIONS 5 //Ilosc iteracji do wykonania przez liscie 


void create_pipes();
void initialize_semaphores();
void binaryTree(int numberOfProcesses, int id);
void close_Root_Pipes(int id);
void close_Parent_Pipes(int id, bool one_subprocess);
void close_Leaf_Pipes(int id);
int * generateData();
int readFromPipe(int pipeId, int nodeArrayToRead[], int processId, int* msgNodeId);
void writeToPipe(int nodeArrayToRead[], int n, int processId, int* msgNodeId);
void* thread_func(void* thread_data);


void handle_sigtstp(int sig);
static int get_shared_block(char *filename, int size);
char * attach_memory_block(char *filename, int size);
bool detach_memory_block(char *block);
bool destroy_memory_block(char *filename);

#define BLOCK_SIZE 4096
#define FILENAME "sharedMemoryFile.c"
#define IPC_RESULT_ERROR (-1)

#define SEM_PRODUCER_FNAME "/producer"
#define SEM_CONSUMER_NAME "/consumer"


#endif