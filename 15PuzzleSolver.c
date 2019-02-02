#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define N 4
#define NxN (N*N)
#define TRUE 1
#define FALSE 0

struct node {
	int tiles[N][N];
	int f, g, h;
	short zero_row, zero_column;
	struct node *next;
	struct node *parent;
};

int goal_rows[NxN];
int goal_columns[NxN];
struct node *start,*goal;
struct node *open = NULL, *closed = NULL;
struct node *succ_nodes[4];
pthread_barrier_t barrier_before_filtering, barrier_after_filtering;
int finish=0, multithread=0;

void print_a_node(struct node *pnode) {
	int i,j;
	for (i=0;i<N;i++) {
		for (j=0;j<N;j++) 
			printf("%2d ", pnode->tiles[i][j]);
		printf("\n");
	}
	printf("\n");
}

struct node *initialize(int argc, char **argv){
	int i,j,k,index, tile;
	struct node *pnode;

	pnode=(struct node *) malloc(sizeof(struct node));
	index = 1;
	for (j=0;j<N;j++)
		for (k=0;k<N;k++) {
			tile=atoi(argv[index++]);
			pnode->tiles[j][k]=tile;
			if(tile==0) {
				pnode->zero_row=j;
				pnode->zero_column=k;
			}
		}
	pnode->f=0;
	pnode->g=0;
	pnode->h=0;
	pnode->next=NULL;
	pnode->parent=NULL;
	start=pnode;
	printf("initial state\n");
	print_a_node(start);

	pnode=(struct node *) malloc(sizeof(struct node));
	goal_rows[0]=3;
	goal_columns[0]=3;

	for(index=1; index<NxN; index++){
		j=(index-1)/N;
		k=(index-1)%N;
		goal_rows[index]=j;
		goal_columns[index]=k;
		pnode->tiles[j][k]=index;
	}
	pnode->tiles[N-1][N-1]=0;
	pnode->f=0;
	pnode->g=0;
	pnode->h=0;
	pnode->next=NULL;
	goal=pnode; 
	printf("goal state\n");
	print_a_node(goal);

	return start;
}

void merge_to_open() {
    //put succ_nodes into open based on open priority
    for(int i = 0; i < N; i++){
        if(succ_nodes[i] == NULL){
            continue;
        }
        //Create a new node to insert, as succ_nodes will soon be cleared for next iteration
        struct node *toInsert = (struct node *) malloc(sizeof(struct node));
        memcpy(toInsert->tiles, succ_nodes[i]->tiles, NxN*sizeof(int));
        toInsert->f = succ_nodes[i]->f;
        toInsert->g = succ_nodes[i]->g;
        toInsert->h = succ_nodes[i]->h;
        toInsert->zero_row = succ_nodes[i]->zero_row;
        toInsert->zero_column = succ_nodes[i]->zero_column;
        toInsert->parent = succ_nodes[i]->parent;

        if (open == NULL)
        {
            open = toInsert;
            continue;
        }

        struct node *temp = open;

        int hasInserted = FALSE;

        while(temp != NULL && temp->next != NULL){
            if(toInsert->f < temp->next->f){
                toInsert->next = temp->next;
                temp->next = toInsert;
                hasInserted = TRUE;
                break;
            }
        temp = temp->next;
        }

        //temp should be either the last node, or the node where toInsert should be inserted
        if(hasInserted == FALSE){
            temp->next = toInsert;
        }

    }

}

/*swap two tiles in a node*/
void swap(int row1,int column1,int row2,int column2, struct node * pnode){
    //swap WITHOUT A TEMP VARIABLE YEAH IM THAT COOL
    pnode->tiles[row1][column1] += pnode->tiles[row2][column2];
    pnode->tiles[row2][column2] = pnode->tiles[row1][column1] - pnode->tiles[row2][column2];
    pnode->tiles[row1][column1] -= pnode->tiles[row2][column2];
}

//used for h2
int manhattanDist(int entry, int row, int col){
    //ignore 0
    if(entry == 0){
        return 0;
    }
    //find entry in goal
    for(int i = 0; i < NxN; i++){
        for(int j = 0; j < NxN; j++){
            if(goal->tiles[i][j] == entry){
                //Goal destination is i, j
                return abs(row - i) + abs(col - j);
            }
        }
    }
}

//used to decide between h1 and h2
int max(int a, int b){
    if(a > b){
        return a;
    }else{
        return b;
    }
}

/*update the f,g,h function values for a node */
void update_fgh(struct node *pnode){
    //g is amount of steps from top
    if(pnode->parent != NULL){
        pnode->g = pnode->parent->g + 1;
    }else{
        pnode->g = 1;
    }

    //h is the max of h1 or h2
    int h1 = 0, h2 = 0;
    int i, j;
    int correct = 0;
    //h1 is number of misplaced tiles, h2 is how far the tile is from desired location
    for(i = 0; i < NxN; i++){
        for(j = 0; i < NxN; i++){
            correct++;
            if(pnode->tiles[j][i] != correct ){
                h1++;
            }
            h2 += manhattanDist(pnode->tiles[j][i], j, i);
        }
    }
    pnode->h = max(h1, h2);

    //f is g + h
    pnode->f = pnode->g + pnode->h;
}



/* 0 goes down by a row */
void move_down(struct node * pnode){
    if(pnode->zero_row+1 < N){
        swap(pnode->zero_row, pnode->zero_column, pnode->zero_row+1, pnode->zero_column, pnode);
        pnode->zero_row++;
    }else{
        pnode = NULL;
    }
}

/* 0 goes right by a column */
void move_right(struct node * pnode){
    if(pnode->zero_column+1 < N){
        swap(pnode->zero_row, pnode->zero_column, pnode->zero_row, pnode->zero_column+1, pnode);
        pnode->zero_column++;
    }else{
        pnode = NULL;
    }
}

/* 0 goes up by a row */
void move_up(struct node * pnode){
    if(pnode->zero_row-1 > -1){
        swap(pnode->zero_row, pnode->zero_column, pnode->zero_row-1, pnode->zero_column, pnode);
        pnode->zero_row--;
    }else{
        pnode = NULL;
    }

}

/* 0 goes left by a column */
void move_left(struct node * pnode){
    if(pnode->zero_column-1 > -1) {
        swap(pnode->zero_row, pnode->zero_column, pnode->zero_row, pnode->zero_column - 1, pnode);
        pnode->zero_column--;
    }else{
        pnode = NULL;
    }
}

/* expand a node, get its children nodes, and organize the children nodes using
 * array succ_nodes.
 */
void expand(struct node *selected) {
    for(int i = 0; i < N; i++){
        succ_nodes[i] = (struct node *) malloc(sizeof(struct node));
        memcpy(succ_nodes[i]->tiles, selected->tiles, NxN*sizeof(int));
        succ_nodes[i]->zero_row = selected->zero_row;
        succ_nodes[i]->zero_column = selected->zero_column;
        succ_nodes[i]->parent = selected;

    }

    move_down(succ_nodes[0]);
    move_right(succ_nodes[1]);
    move_up(succ_nodes[2]);
    move_left(succ_nodes[3]);

    for(int i = 0; i < N; i++){
        update_fgh(succ_nodes[i]);
    }
}

int nodes_same(struct node *a,struct node *b) {
	int flg=FALSE;
	if (memcmp(a->tiles, b->tiles, sizeof(int)*NxN) == 0)
		flg=TRUE;
	return flg;
}

void filter(int i, struct node *pnode_list){
    if(pnode_list == NULL || succ_nodes[i] == NULL){
        return;
    }
    struct node *temp = pnode_list;
    while(temp != NULL){
        if(nodes_same(succ_nodes[i], temp)){
            succ_nodes[i] = NULL;
            return;
        }
        temp = temp->next;
    }
}

void *filter_threads(void *id){
	int *myid = (int *)id;
	while(1){
        pthread_barrier_wait(&barrier_before_filtering);
        if(finish == TRUE){
            pthread_exit(NULL);
        }
		filter(*myid, open);
		filter(*myid, closed);
        pthread_barrier_wait(&barrier_after_filtering);

    }
}

int main(int argc,char **argv) {
	int iter,cnt;
	struct node *copen, *cp, *solution_path;
	pthread_t thread[N-1];
	int ret, i, pathlen=0, index[N-1];

	solution_path=NULL;
	if(strcmp(argv[1], "-s") == 0){
        multithread = 0;
	}else if(strcmp(argv[1], "-m") == 0){
        multithread = 1;
	}else{
        printf("incorrect threading option, first argument must be (-s/-m), not %s", argv[1]);
        return 0;
	}
	start=initialize(argc-1,argv+1);
	open=start; 
	if(multithread){
        pthread_barrier_init(&barrier_before_filtering,NULL,N);
        pthread_barrier_init(&barrier_after_filtering,NULL,N);
        int id[N-1];
        for(i = 0; i < N-1; i++){
            id[i] = i+1;
            pthread_create(&thread[i], NULL, filter_threads, &id[i]);
	    }
	}

	iter=0; 
	while (open!=NULL) {
		copen=open;
		open=open->next;
		if(nodes_same(copen,goal)){
		    /* goal is found */
			if(multithread){
                finish=1;
                pthread_barrier_wait(&barrier_before_filtering);
            }
			do{
				copen->next=solution_path;
				solution_path=copen;
				copen=copen->parent;
				pathlen++;
			} while(copen!=NULL);
			printf("Path (lengh=%d):\n", pathlen); 
            /* print out the nodes on the list */
            do{
                print_a_node(solution_path);
                solution_path = solution_path->next;
            }while(solution_path != NULL);
			break;
		}
		expand(copen);
		if(multithread){
			/* barrier sync */
            pthread_barrier_wait(&barrier_before_filtering);
            filter(0,open);
			filter(0,closed);
            pthread_barrier_wait(&barrier_after_filtering);
            /* barrier sync */
		}
		else{
			for(i=0;i<4;i++){
				filter(i,open);
				filter(i,closed);
			}
		}
		merge_to_open();
		copen->next=closed;
		closed=copen;

		iter++;
		if(iter %1000 == 0){
            printf("iter %d\n", iter);
		}

	}

	if(multithread){
        pthread_barrier_destroy(&barrier_before_filtering);
        pthread_barrier_destroy(&barrier_after_filtering);

        for(i = 0; i < N-1; i++) {
            pthread_join(thread[i], NULL);
        }

    }
	return 0;
}
