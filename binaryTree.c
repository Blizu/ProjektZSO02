#include "binaryTree.h"

int pipes[NUMBER_OF_PROCESSES - 1][2];
sem_t *sem_prod;
sem_t *sem_cons;
int processedData = 0;
enum NodeMsg{END_WORK=-2, REGISTER=-1};


void create_pipes(){
    for (int i = 0; i < NUMBER_OF_PROCESSES; i++){
        if(pipe(pipes[i]) == -1){
            printf("Error podczas tworzenia pipe'a\n");
        }
    }
}

struct ThreadData{
    const int process_id;
    const int input_pipe_id;// output_pipe_id = process_id - 2
    pthread_mutex_t* mutex;
};

struct RootThreadData{
    int* processes_to_end;
    const int process_id;
    const int input_pipe_id;// output_pipe_id = process_id - 2
    pthread_mutex_t* mutex;
};

struct RootSignalThreadData{
    int* processes_to_end;
};

int readFromPipe(int pipeId, int nodeArrayToRead[], int processId, int* msgNodeId){
    int n;
   
       //printf("[Node %d] Czytam poszczegolna wartosc z tablicy\n", processId);
    if(read(pipes[pipeId][0], &n,sizeof(int))<0){
        printf ("Blad odczytu ilosci elementow tablicy z pipe w przypadku node %d\n", processId);
    }
    if (n == REGISTER || n == END_WORK){
        if(read(pipes[pipeId][0], msgNodeId,sizeof(int))<0){
            printf ("Blad odczytu id w node %d\n", processId);
        }
        printf("Odczytano komunikat %d od node %d \n", n, *msgNodeId);
    } else {
        //printf("[Node %d] Czytam tablice z pipe\n", processId);
        if (read(pipes[pipeId][0], nodeArrayToRead, sizeof(int) * n) < 0) {
            printf("Blad odczytu tablicy z pipe w przypadku node %d\n", processId);
        }
    }

    return n;
}

int outputPipeIDFromProcessId(int processId){
    return processId - 2;
}

void forwardMessageFurther(int processId, int n, int* msgNodeId){
    int pipeId = outputPipeIDFromProcessId(processId);
    printf("[Node %d] Zarejestrowano komunikat %d od %d \n", processId, n, *msgNodeId);

    if (write(pipes[pipeId][1], &n, sizeof(int)) < 0) {
        printf("Blad wysylania komunikatu dla liscia o id = %d\n", processId);
    }

    if (write(pipes[pipeId][1], msgNodeId, sizeof(int)) < 0) {
        printf("Blad wysylania id po komunikacie o id = %d\n", processId);
    }
}

void writeToPipe(int nodeArrayToRead[], int n, int processId, int* msgNodeId){
    int pipeId = outputPipeIDFromProcessId(processId);

    if (pipeId < 0){
        if(n == REGISTER){
            printf("[Root] Liść %d zarejestrowano\n", *msgNodeId);
        } else if (n == END_WORK){
            printf("[Root] Liść %d zakończył pracę\n", *msgNodeId);
        } else {
            int childID = nodeArrayToRead[0];
            printf("[Root] Otrzymana tablica od liscia o id %d\n", childID);
            for (int i = 0; i<n; i++) {
                printf("[Root] nodeArrayToRead[%d] = %d\n", i, nodeArrayToRead[i]);
            }
            
        }

    }
    else {        
        if(n == REGISTER || n == END_WORK){
            forwardMessageFurther(processId, n, msgNodeId);
        } else {
            processedData ++;
            //najpierw wysylamy ilosc elementow w tablicy, a potem cala tablice
            if (write(pipes[pipeId][1], &n, sizeof(int)) < 0) {
                printf("Blad wysylania ilosci elementow dla liscia o id = %d\n", processId);
            }
            printf("[Node %d] Wysylam cala tablice \n", processId);
            if (write(pipes[pipeId][1], nodeArrayToRead, sizeof(int) * n) < 0) {
                printf("Blad wysylania tablicy dla lisica o id = %d\n", processId);
            }
        }
    }
}

void* root_thread_func(void* thread_data){
    
    struct RootSignalThreadData* data = (struct RootSignalThreadData*)thread_data;
    while((*data->processes_to_end) > 0){
        sem_wait(sem_cons);
            //printf("Root is in\n");
            char *block = attach_memory_block(FILENAME, BLOCK_SIZE);
            if(block == NULL) { 
                detach_memory_block(block);
            }else if(block == "q"){
                break;
            }
            else{
                printf("ProcessedData \"%s\" dla pid: %d\n", block, getpid());
                detach_memory_block(block);
            }
        sem_post(sem_prod); 
    }

    pthread_exit(NULL);
}

void* root_reader_thread_func(void* thread_data){
    //odczytuje jakie id ma
    struct RootThreadData* data = (struct RootThreadData*)thread_data;
    int nodeArrayToRead[COMMUNICATION_ARRAY_SIZE];
    int n;
    int msgNodeId = -1;
    int observedNodeCounter = 0;
    bool endThis = false;

    while(!endThis) {
        n = readFromPipe(data->input_pipe_id, nodeArrayToRead, data->process_id, &msgNodeId);
        if (n == REGISTER) {
            observedNodeCounter++;
            printf("[Node %d %d] zarejestrowano %d\n", data->process_id, data->input_pipe_id, observedNodeCounter);
        }
        if (n == END_WORK) {
            observedNodeCounter--;
            printf("[Node %d %d] 1 mniej, zarejestrowane: %d\n", data->process_id, data->input_pipe_id, observedNodeCounter);
            if (observedNodeCounter == 0) {
                printf("Pora konczyc ten watek [Node %d %d]\n", data->process_id, data->input_pipe_id);
                endThis = true;
            }
        }

        pthread_mutex_lock(data->mutex);
        writeToPipe(nodeArrayToRead, n, data->process_id, &msgNodeId);
        pthread_mutex_unlock(data->mutex);
    }

    pthread_mutex_lock(data->mutex);
    (*(data->processes_to_end))--;
    pthread_mutex_unlock(data->mutex);

    pthread_exit(NULL);
}

void* thread_func(void* thread_data){
    //odczytuje jakie id ma
    struct ThreadData* data = (struct ThreadData*)thread_data;
    int nodeArrayToRead[COMMUNICATION_ARRAY_SIZE];
    int n;
    int msgNodeId = -1;
    int observedNodeCounter = 0;
    bool endThis = false;

    while(!endThis) {
        n = readFromPipe(data->input_pipe_id, nodeArrayToRead, data->process_id, &msgNodeId);
        if (n == REGISTER) {
            observedNodeCounter++;
            printf("[Node %d %d] zarejestrowano %d\n", data->process_id, data->input_pipe_id, observedNodeCounter);
        }
        if (n == END_WORK) {
            observedNodeCounter--;
            printf("[Node %d %d] 1 mniej, zarejestrowane: %d\n", data->process_id, data->input_pipe_id, observedNodeCounter);
            if (observedNodeCounter == 0) {
                printf("Pora konczyc ten watek [Node %d %d]\n", data->process_id, data->input_pipe_id);
                endThis = true;
            }
        }

        pthread_mutex_lock(data->mutex);
        writeToPipe(nodeArrayToRead, n, data->process_id, &msgNodeId);
        pthread_mutex_unlock(data->mutex);
    }

    pthread_exit(NULL);
}

struct ThreadData generateThreadData(int which, int process_id, pthread_mutex_t* mutex){ //which = 2 - left, 1 - right
    const int input_pipe_id = process_id*2 - which;
    struct ThreadData data = {.process_id = process_id, .mutex = mutex, .input_pipe_id = input_pipe_id};
    return data;
}

struct RootThreadData generateRootThreadData(int which, int process_id, pthread_mutex_t* mutex, int* processes_to_end){ //which = 2 - left, 1 - right
    const int input_pipe_id = process_id*2 - which;
    struct RootThreadData data = {
            .process_id = process_id,
            .mutex = mutex,
            .input_pipe_id = input_pipe_id,
            .processes_to_end = processes_to_end
    };
    return data;
}

struct RootThreadData generateRootLeftThreadData(int process_id, pthread_mutex_t* mutex, int* processes_to_end){
    return generateRootThreadData(2, process_id, mutex, processes_to_end);
}

struct RootThreadData generateRootRightThreadData(int process_id, pthread_mutex_t* mutex, int* processes_to_end){
    return generateRootThreadData(1, process_id, mutex, processes_to_end);
}

struct ThreadData generateLeftThreadData(int process_id, pthread_mutex_t* mutex){
    return generateThreadData(2, process_id, mutex);
}

struct ThreadData generateRightThreadData(int process_id, pthread_mutex_t* mutex){
    return generateThreadData(1, process_id, mutex);
}

void sendMassage(enum NodeMsg nodeMsg, int id){
    printf("Wysylam komunikat %d id = %d\n", nodeMsg, id);
    if(write(pipes[id-2][1],&nodeMsg,sizeof(int))<0){ //Komunikat konca
        printf("Blad wyslania komunikatu %d id = %d\n", nodeMsg, id);

    }

    if(write(pipes[id-2][1],&id,sizeof(int))<0){ //Komunikat konca
        printf("Blad wyslania id po komunikacie %d id = %d\n", nodeMsg, id);
    }
}

void binaryTree(int numberOfProcesses, int id){ //id odlicza ile procesów zostało do stworzenia
    int pid_left = 0, pid_right = 0;
    bool main_process = true; // ten który zaczyna tworzenie podprocesów

    if(id * 2 < numberOfProcesses){ // tworzy dwa podprocesy
        if((pid_left = fork()) == -1){
            printf("Error podczas tworzenia lewego forka pod numerem %d\n", id);
            exit(1);
        }
        if(pid_left == 0){ //To czesc wykonuje tylko dziecko
            id *= 2;
            main_process = false;
            binaryTree(numberOfProcesses, id); // w tym czasie proces staje się "main"
        }
         if(pid_left != 0){
            if((pid_right = fork()) == -1){
                printf("Error podczas tworzenia prawego forka pod numerem %d\n", id);
                exit(1);
            }
        }
         if(pid_right == 0 && pid_left != 0){
            id = id*2 + 1;
            main_process = false;
            binaryTree(numberOfProcesses, id);
        }

    }else if (id * 2 == numberOfProcesses){ // tworzy jeden podproces
        if((pid_left = fork()) == -1){
               printf("Error podczas tworzenia lewego forka pod numerem %d\n", id);
            exit(1);
        }
        if(pid_left == 0){
            id *= 2;
            main_process = false;
            binaryTree(numberOfProcesses,id);
        }
    }

    if(id == 1){
        printf("Jestem rootem id = %d, o pid: %d, ppid: %d\n", id, getpid(),getppid());
        int number_of_child = numberOfProcesses==2 ? 1 : 2;
        int processes_to_end;
        pthread_mutex_t node_mutex = PTHREAD_MUTEX_INITIALIZER;
        //close_Root_Pipes(id);
        if (number_of_child == 1){
            processes_to_end = 1;
            struct RootThreadData data_l = generateRootLeftThreadData(id, &node_mutex, &processes_to_end);
            pthread_t readerLeft;
            pthread_create(&readerLeft, NULL, &root_reader_thread_func, (void*)&data_l);
            pthread_join(readerLeft, NULL);
        } else {
            processes_to_end = 2;
            struct RootThreadData data_l = generateRootLeftThreadData(id, &node_mutex, &processes_to_end);
            struct RootThreadData data_r = generateRootRightThreadData(id, &node_mutex, &processes_to_end);
            //struct RootSignalThreadData root_signal_data = {.processes_to_end = &processes_to_end};

            pthread_t readerLeft, readerRight;
            //pthread_t rootSharedMem;
            pthread_create(&readerLeft, NULL, &root_reader_thread_func, (void *) &data_l);
            pthread_create(&readerRight, NULL, &root_reader_thread_func, (void *) &data_r);
           // pthread_create(&rootSharedMem, NULL, &root_thread_func, &root_signal_data);

            pthread_join(readerLeft, NULL);
            pthread_join(readerRight, NULL);
            //pthread_join(rootSharedMem, NULL);
        }

        struct RootSignalThreadData root_signal_data = {.processes_to_end = &processes_to_end};
        pthread_t rootSharedMem;    
        pthread_create(&rootSharedMem, NULL, &root_thread_func, &root_signal_data);
        pthread_join(rootSharedMem, NULL);
    

    }else if (id * 2 <= numberOfProcesses && main_process){ //Obsluga nodow
        printf("Jestem procesem o id: %d i pid: %d ppid: %d\n", id, getpid(),getppid());
        bool one_subprocess = id*2 == numberOfProcesses ? true : false;
        //close_Parent_Pipes(id, one_subprocess);

        pthread_mutex_t node_mutex = PTHREAD_MUTEX_INITIALIZER;

        struct ThreadData data_l = generateLeftThreadData(id, &node_mutex);
        pthread_t readerLeft;
        pthread_create(&readerLeft, NULL, &thread_func, (void*)&data_l);

        if(!one_subprocess) {
            struct ThreadData data_r = generateRightThreadData(id, &node_mutex);
            pthread_t readerRight;
            pthread_create(&readerRight, NULL, &thread_func, (void *) &data_r);

            pthread_join(readerRight, NULL);
        }

        pthread_join(readerLeft, NULL);

    }else if (id * 2 >  numberOfProcesses && main_process){
        //close_Leaf_Pipes(id);
        printf("Jestem liściem o id: %d i pid: %d, ppid: %d\n", id, getpid(),getppid());
        srand(time(NULL));
        //int numberOfIterations = rand() % 5 + 1; 
        int numberOfIterations = NUMBER_OF_ITERATIONS;
        int n = COMMUNICATION_ARRAY_SIZE;
        int childArray[n];

        sendMassage(REGISTER, id);
        usleep(2000000); //wait for all process to register
        while ((processedData < numberOfIterations)){
       
            childArray[0]=id;
            //printf("childArray %d [%d] = %d\n",id,0,childArray[0]);
            for (int i = 1; i<n; i++){
               childArray[i]= rand() % 11 + id;
               //printf("childArray %d [%d] = %d\n",id,i,childArray[i]);
            }
            //najpierw wysylamy ilosc elementow w tablicy, a potem cala tablice
            if(write(pipes[id-2][1],&n,sizeof(int))<0){
                printf("Blad wysylania ilosci elementow dla liscia o id = %d\n", id);
            }
            //printf("[Leaf %d] Wysylam cala tablice \n",id);
            if(write(pipes[id-2][1],childArray, sizeof(int)*n)<0){
                printf("Blad wysylania tablicy dla lisica o id = %d\n", id);
            } else {
                processedData += 1;
            }
            usleep(SLEEPTIME);
        }
        sendMassage(END_WORK, id);
    }
}

void close_Root_Pipes(int id){ // 0 -> Read, 1 -> Write
    printf("[Close_Root_Pipes] rozpoczecia dzialania\n");
    for(int i = 0; i <= NUMBER_OF_PROCESSES-2; i++){
        //printf("[Close_Root_Pipes] My iteration : %d\n", i);
        if(i != id - 1 && i != id){
            printf("[Close_Root_Pipes] Zamykam pipes [%d][0]\n",i);
            if(close(pipes[i][0]) == -1){ //otwieram [0][0] i [1][0]
                printf("Error with closing pipes in root on i = %d\n",i);
                exit(-2);
            }    
        }
        printf("[Close_Root_Pipes] Zamykam pipes [%d][1]\n",i);
        close(pipes[i][1]);
    }
}

void close_Parent_Pipes(int id, bool one_subprocess){
    for(int i = 0; i < NUMBER_OF_PROCESSES-2; i++){
        if(i != id/2 && id != 2){
            printf("[close_Parent_Pipes %d] Zamykam pipes [%d][1]\n",id,i);
            close(pipes[i][1]);
        }else if(i != id/2 -1 && id == 2){
            printf("[close_Parent_Pipes %d] Zamykam pipes [%d][1]\n",id,i);
            close(pipes[i][1]);
        }

        if(one_subprocess){ // otwiera jeden pipe
            if(i != (id-1)*2){
                printf("[close_Parent_Pipes %d] Zamykam pipes [%d][0]\n",id,i);
                close(pipes[i][0]);
            }
        }else{
            if(i != (id-1)*2 && i != (id-1)*2 + 1){ // otwieram dwa pipe'y dla podprocesów
                printf("[close_Parent_Pipes %d] Zamykam pipes [%d][0]\n",id,i);
                close(pipes[i][0]);
            }
        }
    }
    printf("Wezel %d zamknal pipy\n", id);
}

void close_Leaf_Pipes(int id){
    for(int i = 0; i <= NUMBER_OF_PROCESSES-2; i++){
        
        if(i != id/2 && id != 2){
            printf("[close_Leaf_Pipes %d] Zamykam pipes [%d][1]\n",id,i);
            close(pipes[i][1]);
        }else if(i != id/2 -1 && id == 2){
            printf("[close_Leaf_Pipes %d] Zamykam pipes [%d][1]\n",id,i);
            close(pipes[i][1]);
        }
        printf("[close_Leaf_Pipes %d] Zamykam pipes [%d][0]\n",id,i);
        close(pipes[i][0]);
    }
}

void initialize_semaphores(){
    sem_unlink(SEM_CONSUMER_NAME);
    sem_unlink(SEM_PRODUCER_FNAME);

    sem_prod = sem_open(SEM_PRODUCER_FNAME, O_CREAT | O_EXCL, 0660, 1);
    if (sem_prod == SEM_FAILED){
        //printf("sem_open/producer\n");
        exit(EXIT_FAILURE);
    }

    sem_cons = sem_open(SEM_CONSUMER_NAME, O_CREAT | O_EXCL, 0660, 0);
    if (sem_cons == SEM_FAILED){
        //printf("sem_open/consumer");
        exit(EXIT_FAILURE);
    }
}

void handle_sigtstp(int sig){
    sem_wait(sem_prod);
        char *block = attach_memory_block(FILENAME, BLOCK_SIZE);
        if(block == NULL) { 
            printf("ERROR: couldn't get block\n");    
        }
        char* string[sizeof(int)];
        printf("Pisanie \"%d\" od pid: %d\n", processedData, getpid());
        sprintf(*string, "%d", processedData);
        strncpy(block,*string, BLOCK_SIZE);
        detach_memory_block(block);
    sem_post(sem_cons);
}

static int get_shared_block(char *filename, int size){
    key_t key;

    key = ftok(filename, 0);
    if (key == IPC_RESULT_ERROR){ 
        printf("Error with getting key? filename is %s\n", FILENAME);
        return IPC_RESULT_ERROR; 
    }
    
    return shmget(key, size, 0644 | IPC_CREAT);
    
}

char * attach_memory_block(char *filename, int size){
    int shared_block_id = get_shared_block(filename, size);
    char *result;

    if(shared_block_id == IPC_RESULT_ERROR){ 
        return NULL;
    }

    result = shmat(shared_block_id, NULL, 0);
    if(result == (char *)IPC_RESULT_ERROR) { 
        return NULL;
    }

    return result;
}

bool detach_memory_block(char *block){
    return (shmdt(block) != IPC_RESULT_ERROR);
}