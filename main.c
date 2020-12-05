#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFFER_SIZE 32
#define MAP_SIZE 8
#define SA struct soccaddr
#define 2777

pthread_mutex_t lock;

/* Node Structure that the HashMap will contain */
typedef struct {
    int key;
    char fileName[];
    char contents[];
    node * next;
} node;

/*Structure for the actual Map that will hold the nodes, max of 8 */
typedef struct {
    int size = MAP_SIZE;
    node * headNode;
} map;

int getHashKey(char fileName[]) {
    int hashCode = 0;
    int length = 0;
    for(int i = 0; i < BUFFER_SIZE; i++) {
        if(fileName[i] == "/n" || fileName[i] == "/0") {
            break;
        }
        hashCode += fileName[i] * 19^(2 * i - i);
        length = i;
    }
    return hashCode % length;
}

//make sure that the lock should be applied to all of these methods and not just delete and load
void * loadFile(void * fileName) {
    pthread_mutex_lock(&lock);
    //take in the file name and use it to find the contents of the file
    int hashKey = getHashKey(fileName);

    //use the hash key to return the contents of the file

    pthread_mutex_unlock(&lock);
}

void * deleteCache(void * fileName) {
    pthread_mutex_lock(&lock);
    //take in the file name, get the key, and then remove the file from the cache
    int hashKey = getHashKey(fileName);

    //remove the file from the cache using the key

    pthread_mutex_unlock(&lock);
}

void * storeToCache(){
    pthread_mutex_lock(&lock);
    //store the contents to the cache - create a key


    pthread_mutex_unlock(&lock);
}

int main(int argc, char * argv[]) {
    pthread_t cacheThread;

    char input[1024];
    char * token;
    char command[BUFFER_SIZE];
    char fileName[BUFFER_SIZE] = "";
    char contents[960] = "";

    //initialize the mutex that will be used for the methods (add error handling?)
    pthread_mutex_init(&lock, NULL);

    //start tcp connection and have it always listening for dispatcher *********
    //when something is received, perform all of the below (may move later)


    //when something received, tokenize the string delimiting by spaces
    token = strtok(input, " ");
    strcpy(command, token);

    //further tokenize to get the other strings
    while(token != NULL){
        token = strtok(NULL, " ");

        //check if the filename or contents has a value yet
        if(strlen(fileName) == 0){
            strcpy(fileName, token);
        }else if(strlen(contents) == 0){
            strcpy(contents, token);
        }else{
            break;
        }
    }


    //check what the command was and fire off thread
    if(strcmp(command, "load")){
        //load file from cache
        pthread_create(&cacheThread, NULL, loadFile, (void *) &fileName);

        //return 0 if file not found

    }else if(strcmp(command, "store")){
        //store file in cache -- pass filename and contents


        pthread_create(&cacheThread, NULL, storeToCache, (void *) &fileName);

    }else if(strcmp(command, "rm")){
        //remove file from cache
        pthread_create(&cacheThread, NULL, deleteCache, (void *) &fileName);

    }else{
        //invalid command

    }
}