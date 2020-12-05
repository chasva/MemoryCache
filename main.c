#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 32
#define MAP_SIZE 8

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

void * loadFile(void * fileName) {

}

void * deleteCache(void * fileName) {

}



int main(int argc, char * argv[]) {
    char input[BUFFER_SIZE];



}