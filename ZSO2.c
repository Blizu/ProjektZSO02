#include "binaryTree.h"

int main(int argc, char* argv[])
{
    struct sigaction sa;
    sa.sa_handler = &handle_sigtstp;
    sa.sa_flags = SA_RESTART;
    //sigaction(SIGTSTP, &sa, NULL);
    //sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    int root_pid = getpid();
    create_pipes();

    initialize_semaphores();
    
    binaryTree(NUMBER_OF_PROCESSES, 1);
    
}