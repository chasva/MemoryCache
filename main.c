#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define BUFFER_SIZE 32
#define MAP_SIZE 8

/* Node Structure that the HashMap will contain */
typedef struct {
    int key;
    char * fileName;
    char * contents;
    node * next;
} node;

/*Structure for the actual Map that will hold the nodes, max of 8 */
typedef struct {
    node * headNode;
} table;

/*Method to Create the Table that will be Global Variable */
table createTable() {
    table t;
    node tempHead{-1, "", ""};
    t.headNode = tempHead;
    return t;
}
/*Global Variable so all threads can access it */
table hashTable = createTable();

/*Returns a unique HashKey based on Filename */
int getHashKey(char * fileName) {
    int hashCode = 0;
    length = strlen(fileName);
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

/*Returns a char* to the contents of file, if none exist it returns an empty pointer */
char * getContents(int key) {
    currentNode = hashTable.headNode;
    for(int i = 0; i < MAP_SIZE; i++) {
        if(key == currentNode.key) {
            return currentNode.contents;
        }
        if(currentNode.next != NULL) {
            currentNode = currentNode.next;
        }
    }
    char * empty = ""
    return char empty;
}

void * loadFile(void * fileName) {

}

void * deleteCache(void * fileName) {

}

int main(int argc, char * argv[]) {
    char input[BUFFER_SIZE];



}