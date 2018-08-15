#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#define SIZE 1000
#define inliner __attribute__((always_inline))

struct queue {
    int items[SIZE];
    int front;
    int rear;
};

struct node
{
    int vertex;
    struct node* next;
};


struct Graph
{
    int numVertices;
    struct node** adjLists;
    int* visited;
};

struct queue* createQueue();
struct node* createNode(int);
struct Graph* createGraph(int vertices);

void enqueue(struct queue* q, int);
int dequeue(struct queue* q);
void addEdge(struct Graph* graph, int src, int dest);
void bfs(struct Graph* graph, int startVertex);

struct node* createNode(int v)
{
    struct node* newNode = (node*) malloc(sizeof(struct node));
    newNode->vertex = v;
    newNode->next = NULL;
    return newNode;
}
 

struct Graph* createGraph(int vertices)
{
    struct Graph* graph = (Graph*) malloc(sizeof(struct Graph));
    graph->numVertices = vertices;
 
    graph->adjLists = (node**) malloc(vertices * sizeof(struct node*));
    graph->visited = (int*) malloc(vertices * sizeof(int));
 
    int i;
    for (i = 0; i < vertices; i++) {
        graph->adjLists[i] = NULL;
        graph->visited[i] = 0;
    }
 
    return graph;
}
 
void addEdge(struct Graph* graph, int src, int dest)
{
    // Add edge from src to dest
    struct node* newNode = createNode(dest);
    newNode->next = graph->adjLists[src];
    graph->adjLists[src] = newNode;
 
    // Add edge from dest to src
    newNode = createNode(src);
    newNode->next = graph->adjLists[dest];
    graph->adjLists[dest] = newNode;
}

struct queue* createQueue() {
    struct queue* q = (queue*) malloc(sizeof(struct queue));
    q->front = -1;
    q->rear = -1;
    return q;
}


inliner void enqueue(struct queue* q, int value){
    if(q->rear == SIZE-1)
      q = NULL;
    else {
        if(q->front == -1)
            q->front = 0;
        q->rear++;
        q->items[q->rear] = value;
    }
}

inliner int dequeue(struct queue* q){
    int item;
    if(q->rear == -1){
        item = -1;
    }
    else{
        item = q->items[q->front];
        q->front++;
        if(q->front > q->rear){
            q->front = q->rear = -1;
        }
    }
    return item;
}



void _kernel_bfs(struct Graph* graph, struct queue* q, int startVertex) {
  graph->visited[startVertex] = 1;
  enqueue(q, startVertex);

  while(q->rear != -1){
    int currentVertex = dequeue(q);
    struct node* temp = graph->adjLists[currentVertex];

    while(temp) {
      int adjVertex = temp->vertex;
      if(graph->visited[adjVertex] == 0){
          graph->visited[adjVertex] = 1;
          enqueue(q, adjVertex);
      }
      temp = temp->next;
    }
  }
}

int main()
{
    struct Graph* graph = createGraph(6);
    addEdge(graph, 0, 1);
    addEdge(graph, 0, 2);
    addEdge(graph, 1, 2);
    addEdge(graph, 1, 4);
    addEdge(graph, 1, 3);
    addEdge(graph, 2, 4);
    addEdge(graph, 3, 4);
    struct queue* q = createQueue();
    _kernel_bfs(graph, q, 0);
 
    return 0;
}