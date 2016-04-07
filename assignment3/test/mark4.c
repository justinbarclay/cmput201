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
    int            indegree;
    struct tree   *child[7];
    struct tree   *parent;
};

struct dtree {
    int            index;
    Path           path;
    int            overlap;
    int            totalOverlap;
    struct dtree   *xTraversal;
    struct dtree   *yTraversal;
    struct dtree   *parent;
};

struct overlapPaths {
    int    index[7];
    Path   paths[7];
    int    overlap[7];
    int     length;
};

typedef struct dtree dTree; //a tree that tracks all possible choices for a particular sub-root

typedef struct tree RST;

typedef struct overlapPaths oPaths;

dTree* calcDTree(int** instance, int** MST, int instance_length);

int calcOverlap(Path pathA, Path pathB, int parent);

int findSharedNode(int** MST,int pathIndexA, int pathIndexB);

int findMaxOverlap(dTree* steinerTree, Path currentPath,int** MST, int index);

int findSharedAxis(Path pathA, Path pathB);

dTree* buildDTree(dTree* parent, int index, Path path, int pathLength,int overlap);

void recursiveDTree(dTree* root, int index, int** instance, int** MST,int axis, int length);

bool inRange(int number, int coord1, int coord2);

Path buildPath(int* node1, int* node2, int axis);

void printDTree(dTree* node, int depth);

void printPath(Path path);

void adjustTotalOverlap(dTree* node, int overlap);

int findRoot(int** instance, int length);

RST* buildNode(RST* parent, int index, Path path, int pathLength,int overlap);

void buildTree(RST* root, int** instance, int** mst, int mst_length);

oPaths* findChildOverlaps(int** MST, int** instance,int mst_length, int parentIndex);

oPaths* findOverlappedPaths(dTree* root, oPaths* max);

void freeMST(int** instance, int length);

void printList(RST* node, int depth);

void printTabs(int depth);

int** buildSubMST(int** MST, int mst_length, int parentIndex, int* length);

int findIndex(oPaths* childPaths, int index);

Path copyPath(Path toCopy);

int maxOverlap(RST* root);

void freeDTree(dTree* root);

void freePath(Path path);

void freeRST(RST* root);

int recursiveFindOverlap(dTree* parent, int overlap, Path currentPath);

void addPaths(RST* root, int** instance, int** mst, int mst_length);

void overlapWithChildren(RST* root);

int currentTraversal(Path path);

void recalcOverlap(RST* root);

// int totalDistance(RST* root);
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


    RST* root;
    int rootIndex = findRoot(MST, plane.instance_size-1);
    root = buildNode(NULL, rootIndex, NULL, 0, 0 );

    buildTree(root, plane.instance, MST, plane.instance_size-1);
    addPaths(root, plane.instance, MST, plane.instance_size-1);

    recalcOverlap(root);
    printList(root,0);
    printf("Overlap is %i\n", maxOverlap(root));
    freeMST(MST, plane.instance_size-1);
    freePlane(&plane);
    freeRST(root);


    // need to free MST
    return 0;
}
///////////////////////////////////////////////////////////////////////////
//
// Trees
//
///////////////////////////////////////////////////////////////////////////

dTree* buildDTree(dTree* parent, int index, Path path, int pathLength,int overlap){
    dTree* newNode = malloc(sizeof(dTree));
    newNode->index = index;
    newNode->path  = path;
    newNode->parent = parent;
    newNode->overlap = overlap;
    newNode->totalOverlap = overlap;

    newNode->xTraversal = NULL;
    newNode->yTraversal = NULL;

    return newNode;
}

dTree* calcDTree(int** instance, int** MST, int instance_length){
    dTree* tree = NULL;
    int index =0;
    tree =  buildDTree(NULL, MST[0][0], NULL, 0, 0);
    recursiveDTree(tree, index, instance, MST,0, instance_length);
    recursiveDTree(tree, index, instance, MST,1, instance_length);
    //printDTree(tree, 0);
    return tree;
}

void recursiveDTree(dTree* root, int index, int** instance, int** MST,int axis, int length){
    if(length ==0){
        return;
    } else {
        dTree* newNode;
        int distance = MST[index][2];
        int* nodeA = instance[MST[index][0]];
        int* nodeB = instance[MST[index][1]];

        Path currentPath = buildPath(nodeA, nodeB, axis);

        int overlap = recursiveFindOverlap(root, 0, currentPath);

        newNode = buildDTree(root, MST[index][1], currentPath, distance, overlap);
        adjustTotalOverlap(root, overlap);
        if(axis ==0){
            root->xTraversal = newNode;
        } else {
            root->yTraversal = newNode;
        }

        index++;
        length--;
        recursiveDTree(newNode, index, instance, MST, 0, length);
        recursiveDTree(newNode, index, instance, MST, 1, length);
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
    path[1]=node3;
    path[2]=node2;
    return path;
}

RST* buildNode(RST* parent, int index, Path path, int pathLength,int overlap){
    RST* newNode = malloc(sizeof(RST));
    newNode->index = index;
    newNode->path  = path;
    newNode->pathLength = pathLength;
    newNode->parent = parent;
    newNode->overlap = overlap;
    newNode->indegree=0;

    for(int i=0; i< 7; i++){
        newNode->child[i]=NULL;
    }
    return newNode;
}

void buildTree(RST* root, int** instance, int** mst, int mst_length){
    for(int i=0; i< mst_length; i++){
        if(mst[i][0] == root->index){
            //this must contain a child
            RST* child = buildNode(root, mst[i][1],NULL, mst[i][2], 0);
            root->child[root->indegree] = child;
            root->indegree++;
            buildTree(child, instance, mst, mst_length);
        }
    }
    return;
};


void addPaths(RST* root, int** instance, int** mst, int mst_length){
    if(root->indegree==0){
        return;
    } else {
        for(int i=0; i < root->indegree; i++){
            addPaths(root->child[i], instance, mst, mst_length);
        }
        oPaths* childPaths = findChildOverlaps(mst, instance, mst_length, root->index);
        for(int i=0; i < root->indegree; i++){
            RST* child = root->child[i];
            int index = findIndex(childPaths, child->index);
            if(index > -1){
                child->path = childPaths->paths[index];
                child->overlap = childPaths->overlap[index];
            }
            overlapWithChildren(child);
        }
    }
}
oPaths* findChildOverlaps(int** MST, int** instance,int mst_length, int parentIndex){
    int subLength;
    int**  subMST = buildSubMST(MST, mst_length, parentIndex, &subLength);
    if (subLength>0 && subMST != NULL){
        dTree* tree = calcDTree(instance, subMST, subLength);
        oPaths* maximalOverlap = findOverlappedPaths(tree, NULL);
        freeMST(subMST, subLength);
        return maximalOverlap;
    }
    freeMST(subMST, subLength);
    return NULL;
}

oPaths* findOverlappedPaths(dTree* root, oPaths* max){
    if(max==NULL){
        max = malloc(sizeof(oPaths));
        max->length=0;
    }
    if(root->xTraversal == NULL){
        return max;
    } else {
        int yOverlap = root->yTraversal->totalOverlap;
        int xOverlap = root->xTraversal->totalOverlap;

        max->index[max->length] =root->yTraversal->index;


        if(xOverlap > yOverlap){
            max->overlap[max->length] = root->xTraversal->overlap;
            max->paths[max->length] = copyPath(root->xTraversal->path);
            max->length++;
            return findOverlappedPaths(root->xTraversal, max);
        } else{
            max->overlap[max->length] = root->yTraversal->overlap;
            max->paths[max->length] = copyPath(root->yTraversal->path);
            max->length++;
            return findOverlappedPaths(root->yTraversal, max);
        }
    }
}


//////////////////////////////////////////////////////////////////////
//
// Algorithms for finding Overlap
//
//////////////////////////////////////////////////////////////////////
int recursiveFindOverlap(dTree* parent, int overlap, Path currentPath){
    // Recursively traverse up the tree from the current node, and find out what
    // node that child maximally overlaps
    if(parent == NULL || parent->path==NULL){
        return overlap;
    }
    else{
        int currentOverlap = calcOverlap(currentPath, parent->path, 0);
        if (currentOverlap > overlap){
            overlap = currentOverlap;
        }
        return recursiveFindOverlap(parent->parent, overlap, currentPath);
    }
}

void overlapWithChildren(RST* root){
    //this function takes in a node and calculates it's maximalOverlap with it's children
    //given both the parents x and y traversal

    int axis = currentTraversal(root->path);
    Path newPath = buildPath(root->path[0], root->path[2], !axis);

    for(int i=0; i < root->indegree; i++){
        // Go through every child and calculate how it overlaps with the parent
        // if currently and how it would overlap if the parent changed axis
        // traversal. If either are greater than the parents current overlap
        // update the tree accordingly
        int newOverlap=0;
        int currentOverlap=0;
        currentOverlap = calcOverlap(root->path, root->child[i]->path, 1);
        newOverlap = calcOverlap(newPath, root->child[i]->path, 1);

        if(root->child[i]->overlap == 0){
            // If child root is zero, it's safe to change it's traversal axis
            // and see if it has an effect on overlap with parent
            int childAxis = currentTraversal(root->child[i]->path);
            Path newChildPath = buildPath(root->child[i]->path[0], root->child[i]->path[2], !childAxis);
            int newPathNewChild = calcOverlap(newPath, newChildPath, 1);
            int oldPathNewChild = calcOverlap(root->path, newChildPath, 1);

            if(root->overlap > currentOverlap && root->overlap > newOverlap && root->overlap > newPathNewChild && root->overlap > oldPathNewChild){
                // If root overlap provides the most overlap, break
                break;
            } else if(currentOverlap > root->overlap && currentOverlap > newOverlap && currentOverlap > newPathNewChild && currentOverlap > oldPathNewChild){
                // If currentOverlap provides the move overlap, update
                // root->overlap
                root->overlap = currentOverlap;
            } else if(newOverlap > currentOverlap && newOverlap > root->overlap && newOverlap > newPathNewChild && newOverlap > oldPathNewChild){
                // if new root path provides the most overlap, update root
                root->path = newPath;
                root->overlap = newOverlap;
            } else if(newPathNewChild > currentOverlap && newPathNewChild > newOverlap && newPathNewChild > root->overlap && newPathNewChild > oldPathNewChild){
                // handling a special case, if a child has overlap 0, change
                // it's path if it overlaps with newPath update both root and child
                root->path = newPath;
                root->overlap = newOverlap;
                root->child[i]->path = newChildPath;
            } else if(oldPathNewChild > currentOverlap && oldPathNewChild > newOverlap && oldPathNewChild > newPathNewChild && oldPathNewChild > root->overlap){
                // handling a special case, if a child has overlap 0, change
                //it's path if it overlaps with roots old path update root over lap and child path
                root->overlap = newOverlap;
                root->child[i]->path = newChildPath;
            }
        }

        if(root->child[i]->overlap > 0){
            if(currentOverlap > newOverlap && currentOverlap > root->overlap){
                root->overlap = currentOverlap;
            } else if(newOverlap > root->overlap && currentOverlap > root->overlap){
                root->path = newPath;
                root->overlap = newOverlap;
            }
        }
    }
    return;
}

int calcOverlap(Path pathA, Path pathB, int parent){
    // Given two paths this function uses the midpoints of both paths
    // to find out what axis (if any) they have in common
    int* midpointA = pathA[1];
    int* midpointB = pathB[1];

    int sharedAxis = findSharedAxis(pathA, pathB);
    int posSharedA;
    if(parent==1){
        posSharedA = 2;
    } else{
        posSharedA = 0;
    }
    if(sharedAxis==-1 || sharedAxis > 1){
        return 0;
    }
    int otherAxis = !sharedAxis;
    if(midpointA[otherAxis] > pathA[posSharedA][otherAxis] &&
             midpointB[otherAxis] > pathA[posSharedA][otherAxis]){
    // if both midpointA and midpointB exist on the graph after the point of their
    // shared node, there is overlap
    // Subtract the path
        if(midpointA[otherAxis] > midpointB[otherAxis]){
            return midpointB[otherAxis] - pathA[posSharedA][otherAxis];
        } else{
            return midpointA[otherAxis] - pathA[posSharedA][otherAxis];
        }

    } else if(midpointA[otherAxis] < pathA[posSharedA][otherAxis] &&
                 midpointB[otherAxis] < pathA[posSharedA][otherAxis]){
    // if both midpointA and midpointB exist on the graph before the point of their
    // shared node, there is overlap
        // Find which point is closer to their point in the shared node and
        //subtract that from the sharedAxis position
        if(midpointA[sharedAxis] > midpointB[otherAxis]){
            return pathA[posSharedA][otherAxis] - midpointA[otherAxis];
        } else{
            return pathA[posSharedA][otherAxis] - midpointB[otherAxis];
        }
    }
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
// Helper
//
/////////////////////////////////////////////////////////////////////////////
void recalcOverlap(RST* root){
    // Top down traversal of the tree to recalculate overlap
    int maxParentOverlap=0;
    int parentOverlap =0;
    for(int i=0; i< root->indegree; i++){
        int maximalOverlap = 0;
        int newOverlap = 0;
        // find the maximal overlap with each child with every other child

        for(int j=i+1; j<root->indegree; j++)
        // this should ensure that the children never double count each others overlap
            newOverlap = calcOverlap(root->child[i]->path, root->child[j]->path, 0);
                if(newOverlap > maximalOverlap){
                    maximalOverlap = newOverlap;
                }
        }
        root->child[i]->overlap = maximalOverlap;
        recalcOverlap(root->child[i]);
    }

    if(root->path != NULL && root->child[i]!= NULL){
        // If the root has a path and has children, check to see what it's
        // maximal overlap values are with it's children
        parentOverlap = calcOverlap(root->path, root->child[i]->path, 1);
        if(parentOverlap > maxParentOverlap){
            maxParentOverlap = parentOverlap;
        }
    }

    if(maxParentOverlap>root->overlap){
        root->overlap = maxParentOverlap;
    }
}


void adjustTotalOverlap(dTree* node, int overlap){
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

int currentTraversal(Path path){
    // this function finds the initial traversal of a particular path
    // if the path traverses X first it returns 0 and if it traverses y
    // first it returns 1
    // counts parent
    if(path[0][1] == path[1][1]){
        return 0;
    } else {
        return 1;
    }
}

void printTabs(int depth){
    for(int i =0; i< depth; i++){
            printf("\t");
    }
    return;
}

int findIndex(oPaths* childPaths, int index){
    if(childPaths == NULL){
        return -1;
    }
    for(int i=0; i<childPaths->length; i++ ){
        if(childPaths->index[i] == index){
            return i;
        }
    }
    return -1;
}

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

int findSharedNode(int** MST, int pathIndexA, int pathIndexB){
    //find the node that PathA has in common with PathB and return the index of the node form A
    if(MST[pathIndexA][0] == MST[pathIndexB][0] || MST[pathIndexA][0] == MST[pathIndexB][1]){
        return 0;
    } else if(MST[pathIndexA][1] == MST[pathIndexB][0] || MST[pathIndexA][1] == MST[pathIndexB][1]){
        return 1;
    }

    return -1;
}

void printDTree(dTree* node, int depth) {
    dTree* current = node;
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
        printDTree(current->xTraversal, depth);
    }
    if(current->yTraversal != NULL){
        printDTree(current->yTraversal, depth);
    }
}

void printPath(Path path){
    printf("[%i, %i] -> [%i, %i] -> [%i, %i]\n", path[0][0], path[0][1], path[1][0], path[1][1], path[2][0], path[2][1]);
}

Path copyPath(Path toCopy){
    Path copiedPath = malloc(sizeof(int)*6);
    for(int i=0; i<3; i++){
        int* node = malloc(sizeof(int)*2);
        node[0] = toCopy[i][0];
        node[1] = toCopy[i][1];
        copiedPath[i] = node;
    }
    return copiedPath;
}

void printList(RST* node, int depth) {
    // Prints out the contents of an RST struct according to
    // lab 10 specification
    RST* current = node;
    if (depth==0){
        printf("Root is %i\n", current->index);
    }
    printTabs(depth);
    printf("Point %i has distance %i ", current->index, current->pathLength);
    if(current->indegree >0){
        printf("and has in-degree %i:\n", current->indegree);
    } else {
        printf("and has in-degree %i//\n", current->indegree);
    }

    if (depth!=0){
        printTabs(depth);
        printf("Point %i has overlap of %i, using path ", current->index, current->overlap);
        printPath(current->path);

    }
    for(int j=0; j< current->indegree; j++) {
        printList(current->child[j], depth+1);
    }
}

int maxOverlap(RST* root){
    // Traverses a tree and counts up the overlap of each node
    int distance = root->overlap;
    for(int i=0; i<root->indegree; i++){
        distance += maxOverlap(root->child[i]);
    }
    return distance;
}

int** buildSubMST(int** MST, int mst_length, int parentIndex, int* length){
    // Build a SubMST given when the parentIndex is in the first column
    // it returns the MST and updates the length pointer to the length of the
    // array
    // does not count parent
    int count = 0;
    int index = 0;
    //Find the amount of children parent has
    if(mst_length<1){
        return NULL;
    }
    for(int i=0; i<mst_length; i++){
        if(MST[i][0] == parentIndex){
            count++;
        }
    }
    // If an array was created return  it or return NULL
    if(count > 0){
        // make space in new array for X amount of children
        int** subMST = malloc(sizeof(int)*3*count);
        *length = count;
        for(int i=0; i<mst_length; i++){
            if(MST[i][0] == parentIndex){
                // find each child and put them in the new subMST
                int* newEdge = malloc(sizeof(int)*3);
                newEdge[0] = MST[i][0];
                newEdge[1] = MST[i][1];
                newEdge[2] = MST[i][2];

                subMST[index] = newEdge;
                index++;
            }
        }
        return subMST;
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////
//
// Free
//
////////////////////////////////////////////////////////////////////////
void freeMST(int** instance, int length){
    // Free an MST struct
    if (instance != NULL){
        for(int i =0; i< length; i++){
            free(instance[i]);
        }
        free(instance);
    }
    return;
}

void freeDTree(dTree* root){
    // Frees a DTree struct
    if(root == NULL){
        return;
    }
    freeDTree(root->xTraversal);
    freeDTree(root->yTraversal);
    freePath(root->path);
    free(root);
}

void freePath(Path path){
    // Frees a path struct
    if(path == NULL){
        return;
    }
    for(int i=0; i<3; i++){
        free(path[i]);
    }
    free(path);
}

void freeRST(RST* root){
    // Free's an RST structure
    if(root->indegree == 0){
        freePath(root->path);
        free(root);
        return;
    }
    for(int i=0; i<root->indegree; i++){
        freeRST(root->child[i]);
    }
    freePath(root->path);
    if(root == NULL){
        return;
    }
    free(root);
}
