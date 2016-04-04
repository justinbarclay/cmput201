#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include "BST.h"
#include "genPlane.h"
#include "plane.h"

FILE *fp;
/////////////////////////////////////////////////
//
// STRUCTS
//
/////////////////////////////////////////////////

// A path type that should obey the format [node1=[x1,y1], midpoint=[x2,y2], node2=[x3,y3]]

typedef int** Path;

struct tree {
    int            index;
    Path           path;
    int            pathLength;
    int            overlap;
    int            totalOverlap;
    struct tree   *xTraversal;
    struct tree   *yTraversal;
    struct tree   *parent;
};

typedef struct tree RST;



RST* calcRST(int** instance, int** MST, int instance_length);

int findSharedPath(int** MST, int start, int index);

int calcOverlap(Path pathA, int posSharedA, Path pathB);

int findSharedNode(int** MST,int pathIndexA, int pathIndexB);

int findMaxOverlap(RST* steinerTree, Path currentPath,int** MST, int index);

int findSharedAxis(Path pathA, Path pathB);

void calcRSTrecursive(RST* root, int index, int** instance, int** MST,int axis, int length);

bool inRange(int number, int coord1, int coord2);

Path buildPath(int* node1, int* node2, int axis);

RST* buildNode(RST* parent, int index, Path path, int pathLength,int overlap);

void printList(RST* node, int depth);

void printPath(Path path);

void adjustTotalOverlap(RST* node, int overlap);

void printShortestPath(RST* root);

// Must check for errors in instance, if so output error to console.
// If option output is given, output file to a text
// If option output is not given output to screen
int main(int argc, char** argv){
    char* filename = NULL;
    char** lines;
    Plane plane;
    char* options = NULL;
    bool correctFile = true;
    int** MST = NULL;
    int* degrees;
    //initiallize rand() with current time
    srand(time(NULL));

    // Cheching for arguments, if they are greater than or equal to two, assume
    // their are options and a filename being passed in trying to be passed in.
    if(argc >= 2){
        filename = getFilename(argv, argc);
        options = getOption(argv, argc);
    }

    // Grab Data
    if (filename == NULL){
        plane = getParameters();
        plane.instance_size = plane.NUM_PT;
    } else {
        lines = readFile(filename);

        // Check to see if a file was succesffully parsed
        if(lines == NULL){
            printf("File not found\n");
            printf("Exiting...\n");
            return -1;
        }
        plane.generation = getGeneration(filename);
        plane = getFileParameters(lines);
        correctFile = checkFile(plane);
        free(lines);
    }

    // Ensure instances is of the correct size;
    if(!correctFile){
        printf("File is corrupt, the instance file does not match specification\n");
        return -2;
    }


    if(filename != NULL){
    // If we opened up a file, the Plane instance is all ready generated for us.
        printPlane(plane, filename, options);
        if(plane.instance_size > 1){
            MST = prims(plane);
            printMST(MST,plane.instance_size, filename, options);

            degrees = nodeDegree(MST, plane.instance_size-1);
        } else {
            printf("Can not generate MST, insufficient nodes \n");
        }
    } else {
        while(plane.generation < plane.total_gen){
    // If not we need to generate instances for the number of planes required
            plane.instance = genInstance(plane.NUM_PT, plane.MAX_X, plane.MAX_Y);
            printPlane(plane, NULL, options);
            filename = genFilename(plane.NUM_PT, plane.generation);
            if(plane.instance_size > 1){
                MST = prims(plane);
                printMST(MST,plane.instance_size, filename, options);
                degrees = nodeDegree(MST, plane.instance_size-1);
            } else {
                printf("Can not generate MST, insufficient nodes \n");
            }
            plane.generation++;
        }
    }
    RST* tree;
    tree =calcRST(plane.instance, MST, plane.instance_size);
    // Need to create code to free plane.instance and and sub arrays of instance
    //printList(tree,0);
    printShortestPath(tree);
    free(plane.instance);

    // need to free MST
    return 0;
}


RST* calcRST(int** instance, int** MST, int instance_length){
    RST* tree = NULL;
    int index =0;
    tree =  buildNode(NULL, 0, NULL, 0, 0);
    calcRSTrecursive(tree, index, instance, MST,0, instance_length-1);
    calcRSTrecursive(tree, index, instance, MST,1, instance_length-1);
    return tree;
}
// Need to solve where and index comes froms
void calcRSTrecursive(RST* root, int index, int** instance, int** MST,int axis, int length){
    if(length ==0){
        return;
    } else {
        RST* newNode;
        int distance = MST[index][2];
        int* nodeA = instance[MST[index][0]];
        int* nodeB = instance[MST[index][1]];


        Path currentPath = buildPath(nodeA, nodeB, axis);

        int maxOverlap = findMaxOverlap(root, currentPath, MST, index);
        int minimizedDistance = distance - maxOverlap;  //Total distance covered so far

        newNode = buildNode(root, index, currentPath, minimizedDistance, maxOverlap);
        adjustTotalOverlap(root, maxOverlap);
        if(axis ==0){
            root->xTraversal = newNode;
        } else {
            root->yTraversal = newNode;
        }

        index++;
        length--;
        calcRSTrecursive(newNode, index, instance, MST, 0, length);
        calcRSTrecursive(newNode, index, instance, MST, 1, length);
    }
}

int findSharedPath(int** MST, int start, int index){
    int startNode = MST[index][0];
    int endNode = MST[index][1];
    // Grab the X and Y ranges of
    for(int i=start; i<index; i++){
        //for start and end every node in RST check to see if they match with
        //start or end node of Path
        if(MST[i][0] == startNode || MST[i][1] == startNode || MST[i][0] == endNode ||
        MST[i][1] == startNode){
            return i;
        };
    }
    return -1;
}

int calcOverlap(Path pathA, int posSharedA, Path pathB){
    int* midpointA = pathA[1];
    int* midpointB = pathB[1];

    int sharedAxis = findSharedAxis(pathA, pathB);

    if(sharedAxis==-1 || sharedAxis > 1){
        return 0;
    }
    sharedAxis = !sharedAxis;
    if(midpointA[sharedAxis] > pathA[posSharedA][sharedAxis] &&
             midpointB[sharedAxis] > pathA[posSharedA][sharedAxis]){
    // if both midpointA and midpointB exist on the graph after the point of their
    // shared node, there is overlap
    // Subtract the path
        if(midpointA[sharedAxis] > midpointB[sharedAxis]){
            return midpointB[sharedAxis] - pathA[posSharedA][sharedAxis];
        } else{
            return midpointA[sharedAxis] - pathA[posSharedA][sharedAxis];
        }

    } else if(midpointA[sharedAxis] < pathA[posSharedA][sharedAxis] &&
                 midpointB[sharedAxis] < pathA[posSharedA][sharedAxis]){
    // if both midpointA and midpointB exist on the graph before the point of their
    // shared node, there is overlap
        // Find which point is closer to their point in the shared node and
        //subtract that from the sharedAxis position
        if(midpointA[sharedAxis] > midpointB[sharedAxis]){
            return pathA[posSharedA][sharedAxis] - midpointA[sharedAxis];
        } else{
            return pathA[posSharedA][sharedAxis] - midpointB[sharedAxis];
        }
    }
    return 0;
}
int findSharedNode(int** MST,int pathIndexA, int pathIndexB){
    //find the node that PathA has in common with PathB and return the index of the node form A
    if(MST[pathIndexA][0] == MST[pathIndexB][0] || MST[pathIndexA][0] == MST[pathIndexB][1]){
        return 0;
    } else if(MST[pathIndexA][1] == MST[pathIndexB][0] || MST[pathIndexA][1] == MST[pathIndexB][1]){
        return 1;
    }

    return -1;
}
int findMaxOverlap(RST* steinerTree, Path currentPath,int** MST, int index){
    int start=-1;
    int overlap = 0;
    int maxOverlap= 0;
    if(start < index){
        do {
            start++;
            start= findSharedPath(MST, start, index);
            if(start >= 0){
                int sharedAIndex = findSharedNode(MST, index, start);
                overlap = calcOverlap(currentPath, sharedAIndex, steinerTree->path);
            }
            if(overlap > maxOverlap){
                maxOverlap = overlap;
            }
        }while(start != -1 && start < index);
    }
    return maxOverlap;
}

void adjustTotalOverlap(RST* node, int overlap){
    if(node == NULL){
        return;
    }

    node->totalOverlap += overlap;
    adjustTotalOverlap(node->parent, overlap);
}

int findSharedAxis(Path pathA, Path pathB){
    // Returns the shared axis of both Paths, if any exists. Where 0 = x, 1 = y
    // and -1 means a path is not shared.
    // We can tell what axis is shared between two paths by checking if the
    // x or y of their midpoints lie on the same axis
    if(pathA[1][0] == pathB[1][0]){
        return 0;
    } else if(pathA[1][1] == pathB[1][1]){
        return 1;
    }
    return -1;
}

bool inRange(int number, int coord1, int coord2){
    if(coord1 < coord2){
        return (number >= coord1 && number <= coord2);
    } else if(coord1 > coord2) {
        return (number >= coord2 && number <= coord1);
    } else {
        return false;
    }
}

Path buildPath(int* node1, int* node2, int axis){
    int* node3 = malloc(sizeof(int)*2);
    Path path = malloc(sizeof(int)*6);
    if(axis == 0){
        //midpoint on x axis
        node3[0] = node2[0];
        node3[1] = node1[1];
    } else {
        //midpoint on y axis
        node3[0] = node1[0];
        node3[1] = node2[1];
    }
    path[0]=node1;
    path[2]=node2;
    path[1]=node3;
    return path;
}


RST* buildNode(RST* parent, int index, Path path, int pathLength,int overlap){
    RST* newNode = malloc(sizeof(RST));
    newNode->index = index;
    newNode->path  = path;
    newNode->pathLength = pathLength;
    newNode->parent = parent;
    newNode->overlap = overlap;
    newNode->totalOverlap = overlap;

    newNode->xTraversal = NULL;
    newNode->yTraversal = NULL;

    return newNode;
}

/* void buildTree(point* root, int** mst, int mst_length){ */
/*     for(int i=0; i< mst_length; i++){ */
/*         if(mst[i][0] == root->idx){ */
/*             //this must contain a child */
/*             point* child = newPoint(mst[i][1], root, mst[i][2]); */
/*             root->child[root->indegree] = child; */
/*             root->indegree++; */
/*             buildTree(child, mst, mst_length); */
/*         } */
/*     } */
/*     return; */
/* }; */
int findRoot(int** instance, int length){
    int count[length];
    int search;


    // initialize count to 0
    for(int i=0; i<length; i++){
        count[i] = 0;
    }

    // count all the indexes from 0 to length -1
    for(int i=0; i<length; i++){
       count[instance[i][1]]++;
    }

    // search through count for the index parent
    for(int i=0; i<length; i++){
        if(count[i]==0){
            search = count[i];
            break;
        }
    }
    //find this index in instance[i][0] and return it
    for(int i=0; i<length; i++){
        if(instance[i][0] == search){
            return i;
        }
    }
    return -1;
}
void printPath(Path path){
    printf("[%i, %i] -> [%i, %i] -> [%i, %i]\n", path[0][0], path[0][1], path[1][0], path[1][1], path[2][0], path[2][1]);
}
void printList(RST* node, int depth) {
    RST* current = node;
    if (depth==0){
        printf("root is %i\n", current->index);
    }
    printf("\n");
    for(int i =0; i< depth; i++){
            printf("\t");
    }
    printf("Node index: %i\n", current->index);
    for(int i =0; i< depth; i++){
            printf("\t");
    }
    printf("Node distance: %i\n", current->pathLength);
    for(int i =0; i< depth; i++){
            printf("\t");
    }
    printf("Node maxOverlap %i\n", current->overlap);
    for(int i =0; i< depth; i++){
            printf("\t");
    }
    printf("Node totalOverlap %i\n", current->totalOverlap);
    for(int i =0; i< depth; i++){
            printf("\t");
    }
    depth++;
    if(current->path != NULL){
        printPath(current->path);
    }
    if(current->xTraversal != NULL){
        printList(current->xTraversal, depth);
    }
    if(current->yTraversal != NULL){
        printList(current->yTraversal, depth);
    }
}

void printShortestPath(RST* root){
    if (root->xTraversal == NULL){
        return;
    }
    int yOverlap = root->yTraversal->totalOverlap;
    int xOverlap = root->xTraversal->totalOverlap;
    if(xOverlap > yOverlap){
        printPath(root->xTraversal->path);
        printShortestPath(root->xTraversal);
    } else{
        printPath(root->yTraversal->path);
        printShortestPath(root->yTraversal);
    }
}
