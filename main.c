#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#define MAP_SIZE 8
#define SERVER_PORT 1041
#define MESSAGE_SIZE 1024

int serverSocket;
pthread_mutex_t lock;

void closeConnection(){
    close(serverSocket);
    exit(1);
}

//Lock that will be used
pthread_mutex_t lock;

//Method to get the slot in Hashmap
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

//The HashMap-Table thingy
typedef struct {
    node **nodes;
} table;

//Initialize it
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

//Method used to create nodes
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

//Returns the contents of a file
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

    //What to Do if the file isn't found
    char * empty = "0:";
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
                return;
            }
                //Previous node hasn't been declared yet so set slot to null
            else {
                mainTable->nodes[slot] = NULL;
                return;
            }

        }

        previousNode = slotNode;
        slotNode = slotNode->next;
    }

}

//make sure that the lock should be applied to all of these methods and not just delete and load
char * loadFile(void * fileName) {
    //Acquire the Lock
    pthread_mutex_lock(&lock);
    //Calls the file
    char * contents = loadContents(fileName);
    //Release lock
    pthread_mutex_unlock(&lock);
    return contents;
}

void storeToCache(char * fileName, char * contents){
    //Get the Filename and Contents set it
    pthread_mutex_lock(&lock);
    setNode(fileName, contents);
    pthread_mutex_unlock(&lock);
}

void deleteCache(char * fileName) {
    pthread_mutex_lock(&lock);
    removeFile(fileName);
    pthread_mutex_unlock(&lock);
}

node passNode;
//called whenever the client receives a message from the server (dispatcher)
void * messageReceived(void * input){
    char * receiveLine = (char *)input;
    //set to pointers to work better with the hashtable functions
    char * token = NULL;
    char * command = NULL;
    char * fileName = NULL;
    char * contents = NULL;
    char * result = NULL;

    //Freeing the memory for times after first use
    free(result);
    free(contents);
    free(fileName);
    free(token);
    free(command);
    free(passNode.fileName);
    free(passNode.contents);

    //when something received, tokenize the string delimiting by spaces
    token = malloc(strlen(receiveLine) +1);
    token = strtok(receiveLine, " ");
    command = malloc(strlen(token) + 1);
    strcpy(command, token);

    //further tokenize to get the other strings
    while(token != NULL){
        //check if the filename or contents has a value yet
        if(fileName == NULL) {
            token = strtok(NULL, " ");
            fileName = malloc(strlen(token) +1);
            strcpy(fileName, token);
        }
        if(contents == NULL && strcmp(command, "store") == 0){
            //Get the contents before and after the colon
            token = strtok(NULL, "\n");
            contents = malloc(strlen(token) + 1);
            strcpy(contents, token);
        }
        else {
            break;
        }
    }

    //declaring return variable that will be joined to thread


    //check what the command was and fire off thread
    if(strcmp(command, "load") == 0){
        //Get rid of new line character
        strtok(fileName, "\n");

        //Load it
        result = malloc(strlen(loadContents(fileName))+1);
        strcpy(result, loadFile(fileName));

    }else if(strcmp(command, "store") == 0){
        //store file in cache -- pass filename and contents
        storeToCache(fileName, contents);

        //Set result
        result = malloc(strlen("Successful Store") + 1);
        strcpy(result, "Successful Store");

    }else if(strcmp(command, "rm") == 0){
        //Get rid of the new line character
        strtok(fileName, "\n");

        //Remove the file
        deleteCache(fileName);
        result = malloc(strlen("Successful Removal") + 1);
        strcpy(result, "Successful Removal");

    }else{
        //invalid command
        result = malloc(strlen("Invalid Command") + 1);
        strcpy(result, "Invalid Command");
    }

    return result;
}

int main(int argc, char * argv[]) {
    //initialize the mutex that will be used for the methods (add error handling?)
    pthread_mutex_init(&lock, NULL);

    //initialize the hash table
    mainTable = createTable();

    //start tcp connection and have it always listening for dispatcher *********
    int connectionToClient, bytesRead;

    char sendLine[MESSAGE_SIZE];
    char receiveLine[MESSAGE_SIZE];

    //create the socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;

    //listen to any address
    //convert address and ports to network formats
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(SERVER_PORT);

    //bind to the port
    if(bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1){
        printf("Port cannot be bound at this time\n");
        exit(-1);
    }

    //register for Ctrl+C
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = closeConnection;
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    //listen for connections
    listen(serverSocket, 10);

    char * response;

    while(1){
        connectionToClient = accept(serverSocket, (struct sockaddr *) NULL, NULL);

        //Get the request that the client has
        while((bytesRead = read(connectionToClient, receiveLine, MESSAGE_SIZE)) > 0){
            //replace the newline with a null terminator
            receiveLine[bytesRead] = '\0';

            pthread_t cacheThread;
            pthread_create(&cacheThread, NULL, messageReceived, (void *) receiveLine);

            //return 0 if file not found - implement in hash map?
            pthread_join(cacheThread, &response);
            //manage the input

            snprintf(sendLine, sizeof(sendLine), (char *)response);
            write(connectionToClient, sendLine, strlen(sendLine));

            //get rid of artifacts by zeroing out
            bzero(&receiveLine, sizeof(receiveLine));
            close(connectionToClient);
        }
    }
}