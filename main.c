#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 32
#define MAP_SIZE 8
#define SERVER_PORT 2777
#define MESSAGE_SIZE 1024

pthread_mutex_t lock;

/* Node Structure that the HashMap will contain */
typedef struct {
    int key;
    char * fileName;
    char * size;
    char * contents;
    void * next;
} node;

/*Structure for the actual Map that will hold the nodes, max of 8 */
typedef struct {
    int size;
    node * headNode;
} table;

/*Method to Create the Table that will be Global Variable */
table createTable() {
    table t;
    t.size = MAP_SIZE;
    node tempHead = {-1, NULL, NULL, NULL, NULL, NULL};
    t.headNode = &tempHead;
    return t;
}
/*Global Variable so all threads can access it */
table hashTable;

/*Returns a unique HashKey based on Filename */
int getHashKey(char * fileName) {
    int hashCode = 0;
    int length = strlen(fileName);
    char name[length];
    strcpy(name, fileName);

    for(int i = 0; i < BUFFER_SIZE; i++) {
        if(name[i] == "/n" || name[i] == "/0") {
            break;
        }
        hashCode += fileName[i] * 19^(2 * i - i);
        length = i;
    }
    return hashCode % length;
}

/*Returns the size and contents of a file in format n:[contents] n being size, if no contents then it returns 0: */
char * loadContents(int key) {
    //Start at Head Node
    node currentNode = *hashTable.headNode;

    //Iterate through till a match is made
    for(int i = 0; i < MAP_SIZE; i++) {
        if(key == currentNode.key) {

            //Copy the size with room for the contents
            char * loadedContent = malloc(strlen(currentNode.contents) + strlen(currentNode.size) + 1);
            strcpy(loadedContent, currentNode.size);

            //Combine the size with the content
            strcat(loadedContent, currentNode.contents);


            return loadedContent;
        }
        if(currentNode.next == NULL) {
            break;
        }
        currentNode = *(node *)currentNode.next;
    }

    char * empty = "0:";
    return empty;
}

//Used to remove the file node and link one ahead to the rest
void removeNode(int key) {
    //Check if head node needs removal
    node currentNode = *hashTable.headNode;
    if(currentNode.key = key) {
        if(currentNode.next != NULL) {
            //Assign the new head node
            hashTable.headNode = (node *)currentNode.next;

            //Free the memory of the Old Head
            free(currentNode.contents);
            free(currentNode.fileName);
            free(currentNode.size);
            return;
        }
        //If there are no elements behind the head
        else {
            //Remembering to free the memory
            free(currentNode.contents);
            free(currentNode.fileName);
            free(currentNode.size);
            hashTable = createTable();
            return;
        }
    }

    //Assign the nextnode to be used
    node nextNode;

    //Loop through while keeping track of nodes
    for(int i = 0; i < MAP_SIZE; i++) {
        //Continue only if there is a next node
        if(currentNode.next != NULL)
            nextNode = *(node *)currentNode.next;
        else
            return;


        //Enter if it is the node to remove
        if(key == nextNode.key) {
            //Free the memory originally allocated for the pointers
            free(currentNode.contents);
            free(currentNode.fileName);
            free(currentNode.size);

            //Link the previous node to next one if there is to not lose the chain
            if(nextNode.next != NULL)
                currentNode.next = nextNode.next;
            else
                return;

        }
        //Move the node up
        currentNode = *(node *)currentNode.next;

    }
}

//MEthod to add nodes to the hash table
void addToTable(node newNode) {
    node currentNode = *hashTable.headNode;
    for(int i = 0; i < MAP_SIZE; i++) {
        //Check for Collision and Remove it
        if(currentNode.key == newNode.key) {
            //Replace the Node, delete it, and return
            return;
        }
        //Check if the empty node
        if(currentNode.next == NULL) {
            //Link to next node

            //Check how long the list is then delete head if necessary
            if(i >= MAP_SIZE - 1) {
                //Will delete the head and assign the node as necessary
                removeNode(getHashKey(hashTable.headNode->fileName));
            }
        }
        //Break at this point
        if(currentNode.next == NULL) {
            break;
        }
        //Continue through loop
        currentNode = *(node *)currentNode.next;
    }

}
//Create the new Node
void createNode(char * input[]) {
    node newNode;
    newNode.fileName = input[0];
    newNode.key = getHashKey(newNode.fileName);
    newNode.size = input[1];
    newNode.contents = input[2];

    addToTable(newNode);

}

//make sure that the lock should be applied to all of these methods and not just delete and load
void * loadFile(void * fileName) {
    int key = getHashKey((char *)fileName);
    pthread_mutex_lock(&lock);
    //use the hash key to return the contents of the file
    char * file = loadContents(key);
    //Release lock
    pthread_mutex_unlock(&lock);
    return (void *)file;

    /*TODO Remember to free the memory that was allocated for file
     * This should be done after it is sent back to dispatcher or printed or whatever
     * needs to be done*/
}

void * deleteCache(void * fileName) {
    int hashKey = getHashKey(fileName);

    pthread_mutex_lock(&lock);
    //remove the file from the cache using the key
    removeNode(hashKey);
    //release lock
    pthread_mutex_unlock(&lock);
}

void * storeToCache(void * input){
    //Get the Filename, Size, and Contents store it in char * arr called data.
    char * data[3];
    for(int i = 0; i < 3; i++) {
        data[i] = ((char *)input)[i];
    }
    pthread_mutex_lock(&lock);

    createNode(data);

    pthread_mutex_unlock(&lock);
}

//called whenever the client receives a message from the server (dispatcher)
void messageReceived(char * receiveLine){
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

    //free the memory that was used

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
    //snprintf(sendLine, sizeof(sendLine), "Data to be sent");

    //listen for the server
    while((bytesRead = read(serverSocket, receiveLine, MESSAGE_SIZE)) > 0){
        //null terminate the bytes received
        receiveLine[bytesRead] = 0;

        //manage the input
        messageReceived(receiveLine);
    }
}