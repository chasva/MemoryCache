#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>


#define MAP_SIZE 8
#define SERVER_PORT 2777
#define MESSAGE_SIZE 1024

pthread_mutex_t lock;

/* Node Structure that the HashMap will contain */
//Method to get the slot
int getHash(char * fileName) {
    int hashCode = 0;
    int length = strlen(fileName);
    char name[length + 1];
    strcpy(name, fileName);

    for(int i = 0; i < length; i++) {
        if(name[i] == '\n' || name[i] == '\0') {
            break;
        }
        hashCode += fileName[i] * 19^(2 * i - i);

    }
    return hashCode % MAP_SIZE;
}

/* Node Structure that the HashMap will contain */
typedef struct {
    char * fileName;
    char * contents;
    struct node * next;
} node;

typedef struct {
    node **nodes;
} table;

table * createTable() {
    table * hashTable = malloc(sizeof(table) * 1);

    hashTable->nodes = malloc(sizeof(node *) * MAP_SIZE);

    //Set each node to NULL
    for(int i = 0; i < MAP_SIZE; i++) {
        hashTable->nodes[i] = NULL;
    }

    return hashTable;

}

//Creating the global table value
table * mainTable;

node * createNode(char * fileName, char * contents) {
    //Memory Allocation
    node * newNode = malloc(sizeof(newNode) +1);
    newNode->fileName = malloc(strlen(fileName) +1);
    newNode->contents = malloc(strlen(contents) +1);

    //Copying
    strcpy(newNode->fileName, fileName);
    strcpy(newNode->contents, contents);

    //Set the next node as null
    newNode->next = NULL;

    return newNode;
}

/*Method for Setting the Node, Handles Collisions */
void setNode(char * fileName, char * contents) {
    int slot = getHash(fileName);

    //Get the pointer in the slot
    node * slotNode = mainTable->nodes[slot];

    //No Collision
    if(slotNode == NULL) {
        mainTable->nodes[slot] = createNode(fileName, contents);
        return;
    }

    node * previousNode;
    //Collision replacement
    while(slotNode != NULL) {
        //If same filename replace contents
        if(strcmp(fileName, slotNode->fileName) == 0) {
            //Free the old memory
            free(slotNode->contents);
            slotNode->contents = malloc(strlen(contents) + 1);
            strcpy(slotNode->contents, contents);
            return;
        }

        previousNode = slotNode;
        slotNode = slotNode->next;
    }

    //Add to list if at end of chain
    previousNode->next = createNode(fileName, contents);

}

//Returns the contents of a file only, no size
char * loadContents(char * fileName) {
    int slot = getHash(fileName);
    char * contents = NULL;
    node * slotNode = mainTable->nodes[slot];
    while(slotNode != NULL) {
        if(strcmp(slotNode->fileName, fileName) == 0) {
            contents = malloc(strlen(slotNode->contents) + 1);
            strcpy(contents, slotNode->contents);
            return contents;
            //TODO Free the memory that is allocated for it
        }
        slotNode = slotNode->next;
    }
    char * empty = "";
    return empty;
}

//Used to free the memory of file in slot and unlink it
void removeFile(char * fileName) {
    int slot = getHash(fileName);

    //Get the pointer in the slot
    node * slotNode = mainTable->nodes[slot];

    //Nothing exists so return
    if(slotNode == NULL) {
        return;
    }

    node * previousNode = NULL;
    //Collision replacement
    while(slotNode != NULL) {
        //If same filename free the memory
        if(strcmp(fileName, slotNode->fileName) == 0) {
            //Free the memory
            free(slotNode->contents);
            free(slotNode->fileName);

            //Get rid of Link
            if(previousNode != NULL) {
                previousNode->next = slotNode->next;
            }
        }

        previousNode = slotNode;
        slotNode = slotNode->next;
    }

}

//make sure that the lock should be applied to all of these methods and not just delete and load
void * loadFile(void * fileName) {
    //Acquire the Lock
    pthread_mutex_lock(&lock);
    //Calls the file
    char * contents = loadContents((char *)fileName);
    //Release lock
    pthread_mutex_unlock(&lock);
    return (void *)contents;
}


node passNode;
void * storeToCache(void * input){
    //Get the Filename and Contents set it
    node * solution = (node *)input;
    char * fileName = solution->fileName;
    char * contents = solution->contents;

    pthread_mutex_lock(&lock);

    setNode(fileName, contents);

    pthread_mutex_unlock(&lock);

}

void * deleteCache(void * fileName) {
    pthread_mutex_lock(&lock);

    removeFile(fileName);

    pthread_mutex_unlock(&lock);
}

//called whenever the client receives a message from the server (dispatcher)
void * messageReceived(char * receiveLine){
    pthread_t cacheThread;

    //set to pointers to work better with the hashtable functions
    char * token;
    char * command;
    char * fileName = NULL;
    char * contents = NULL;

    //when something received, tokenize the string delimiting by spaces
    token = strtok(receiveLine, " ");
    command = malloc(strlen(token) + 1);
    strcpy(command, token);

    //further tokenize to get the other strings
    while(token != NULL){
        token = strtok(NULL, " ");

        //check if the filename or contents has a value yet
        if(fileName == NULL){
            fileName = malloc(strlen(token) + 1);
            strcpy(fileName, token);
        }else if(contents == NULL){
            contents = malloc(strlen(token) + 1);
            strcpy(contents, token);
        }else{
            break;
        }
    }

    //declaring return variable that will be joined to thread
    void * result;

    //check what the command was and fire off thread
    if(strcmp(command, "load") == 0){
        //load file from cache
        pthread_create(&cacheThread, NULL, loadFile, (void *) &fileName);

        //return 0 if file not found - implement in hash map?
        pthread_join(cacheThread, &result);

    }else if(strcmp(command, "store") == 0){
        //store file in cache -- pass filename and contents


        pthread_create(&cacheThread, NULL, storeToCache, (void *) &fileName);

    }else if(strcmp(command, "rm") == 0){
        //remove file from cache
        pthread_create(&cacheThread, NULL, deleteCache, (void *) &fileName);

    }else{
        //invalid command

    }

    //free the memory that was used; if it's not allocated it'll be ignored anyway
    free(command);
    free(fileName);
    free(contents);

    return result;
}

int main(int argc, char * argv[]) {
    //initialize the mutex that will be used for the methods (add error handling?)
    pthread_mutex_init(&lock, NULL);
    //initialize the hash table
    hashTable = createTable();

    //start tcp connection and have it always listening for dispatcher *********
    int serverSocket, bytesRead;

    char sendLine[MESSAGE_SIZE];
    char receiveLine[MESSAGE_SIZE];

    //create the socket (with error detection)
    if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){

    }

    //set up the server connection
    struct sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    //connect to server
    if(connect(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0){

    }

    //write to socket to transmit to server -- implement this later for returning file information to the dispatcher
    //snprintf(sendLine, sizeof(sendLine), "line to send");
    //write(serverSocket, sendLine, strlen(sendLine));

    char * response;

    //listen for the server
    while((bytesRead = read(serverSocket, receiveLine, MESSAGE_SIZE)) > 0){
        //null terminate the bytes received
        receiveLine[bytesRead] = 0;

        //manage the input
        response = (char *) messageReceived(receiveLine);
        strcpy(sendLine, response);

        //send response
        //snprintf(sendLine, sizeof(sendLine), response);
        write(serverSocket, sendLine, strlen(sendLine));
    }
}