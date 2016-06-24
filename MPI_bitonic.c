#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "mpi.h"

struct timeval startwtime, endwtime;
double seq_time;

int cmpfuncASC (const void * a, const void * b);
int cmpfuncDESC (const void * a, const void * b);
void init(int* a,int seed);

int N;
void test(int* a, int num_tasks,int rank);
void compute_top(int partner, int dir,int *a,int rank, int k);
void compute_bottom(int partner,int dir,int *a,int rank , int k);
inline void swap(int *a, int *b);


/** the main program **/
int main(int argc, char **argv) {

  if (argc != 2) {
    printf("Usage: %s q\n  where n=2^q is problem size (power of two)\n",argv[0]);
    exit(1);
  }
  int  numtasks, rank, rc,i,j, *a, k, dir;
  
  rc = MPI_Init(&argc,&argv);
  if (rc != MPI_SUCCESS) {
     printf ("Error starting MPI program. Terminating.\n");
     MPI_Abort(MPI_COMM_WORLD, rc);
  }
  MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank); 
  
  N = (1<<atoi(argv[1]))/numtasks;
  a = (int *) malloc(N * sizeof(int));  
  init(a,rank);  
  //start counting time
  gettimeofday (&startwtime, NULL);
  if ((rank%2 == 0) ) qsort(a, N, sizeof(int), cmpfuncASC); //ASCENDING
  else qsort(a, N, sizeof(int), cmpfuncDESC); //DESCENDING
  MPI_Barrier(MPI_COMM_WORLD);
  
  for (k=1;k<numtasks;k<<=1){
    dir = ((k*2 & rank) == 0);
   
    for (j=k;j>=1;j>>=1){
      int partner = rank^j;  
      if(rank<partner) compute_bottom(partner,dir,a,rank,j);
      else compute_top(partner,dir,a,rank,j);
      MPI_Barrier(MPI_COMM_WORLD);
    }
    if (dir) qsort(a, N, sizeof(int), cmpfuncASC); //ASCENDING
    else qsort(a, N, sizeof(int), cmpfuncDESC); //DESCENDING
  }
  //program finished
  gettimeofday (&endwtime, NULL);
  seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		      + endwtime.tv_sec - startwtime.tv_sec);

  if(rank == 1) printf("Parallel bitonic sort time = %f\n", seq_time);
  test(a,numtasks,rank);
}

/** function called by the smaller partner to swap the bottom part
    of both tables 
**/
void compute_bottom(int partner, int dir,int *a,int rank, int k){
  MPI_Status mpistat;
  int rc,i;
  int *swap_temp = (int *)malloc(N/2*sizeof(int));
  //tag = 1 means you need to proccess the message. 
  //since my job is to compute the top part, im sending the bottom for the other guy.
  rc = MPI_Send(a+N/2,N/2,MPI_INT,partner,1,MPI_COMM_WORLD);
  if (rc != MPI_SUCCESS) {
    printf ("Error idle. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  //i need to compare this to my bottom half.
  MPI_Recv(swap_temp,N/2,MPI_INT,partner,1,MPI_COMM_WORLD,&mpistat);
  if(dir){
    for(i=0;i<N/2;i++){
      if(a[i] > swap_temp[i]) swap(&a[i],&swap_temp[i]);  
    }
   
  }
  else{
    for(i=0;i<N/2;i++){
      if(a[i] < swap_temp[i]) swap(&a[i],&swap_temp[i]);
    }
  }
  //this guy sends the results before receiving
  rc = MPI_Send(swap_temp,N/2,MPI_INT,partner,2,MPI_COMM_WORLD);
  if (rc != MPI_SUCCESS) {
    printf ("Error idle. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  //receiving the computed top part from the partner.
  MPI_Recv(a+N/2,N/2,MPI_INT,partner,2,MPI_COMM_WORLD,&mpistat);
}  


/** function called by the bigger partner to swap the top part
    of both tables 
**/
void compute_top(int partner, int dir,int *a,int rank, int k) {
  MPI_Status mpistat;
  int rc,i;
  int *swap_temp = (int *)malloc(N/2*sizeof(int));
  //this guy will receive before sending. If they both attemp to send first it may lead to a deadlock.
  MPI_Recv(swap_temp,N/2,MPI_INT,partner,1,MPI_COMM_WORLD,&mpistat);
  rc = MPI_Send(a,N/2,MPI_INT,partner,1,MPI_COMM_WORLD);
  if (rc != MPI_SUCCESS) {
    printf ("Error idle. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  if(dir){ 
      for(i=0;i<N/2;i++){
	if(a[i+N/2] < swap_temp[i]) swap(&a[i+N/2],&swap_temp[i]);
      }   
  }
  else{
    for(i=0;i<N/2;i++){
      if(a[i+N/2] > swap_temp[i]) swap(&swap_temp[i],&a[i+N/2]);
    }
  }
  //Proccessing has ended. This guy will first receive and then send to avoid deadlock.
  MPI_Recv(a,N/2,MPI_INT,partner,2,MPI_COMM_WORLD,&mpistat);
  rc = MPI_Send(swap_temp,N/2,MPI_INT,partner,2,MPI_COMM_WORLD);
  if (rc != MPI_SUCCESS) {
    printf ("Error idle. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }
  
}

// the first element must be the lowest target number
inline void swap(int *a, int *b){

  int t;
  t = *a;
  *a = *b;
  *b = t;
}

void init(int* a,int seed) {
  int i;
  srand(seed+3);
  for (i = 0; i < N; i++) {
    a[i] = rand() % (4*N);
  }
}

int cmpfuncASC (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

int cmpfuncDESC (const void * a, const void * b)
{
   return ( *(int*)b - *(int*)a );
}
void test(int* a, int num_tasks,int rank)
{
  int flag=1;
  int i,rc;
  int min = a[0];
  int max = 0;

  
  for(i=0;i<N-1;i++){
    if (a[i+1] < a[i]) flag = 0;
    if (a[i] > max) max = a[i];
    if (a[i] < min) min = a[i];
  }
  if(rank == 0){
   
    MPI_Status mpistat;
    int other_flag;
    int *minimum = (int *)malloc(num_tasks*sizeof(int));
    int *maximum = (int *)malloc(num_tasks*sizeof(int));
    minimum[0] = min;
    maximum[0] = max;
    for(i=1;i<num_tasks;i++){
      //tag = 3 means local sorting flag
      MPI_Recv(&other_flag,1,MPI_INT,i,3,MPI_COMM_WORLD,&mpistat);
      flag = flag & other_flag;
      //tag = 4 means minimum
      MPI_Recv(minimum+i,1,MPI_INT,i,4,MPI_COMM_WORLD,&mpistat);
      //tag = 5 means maximum
      MPI_Recv(maximum+i,1,MPI_INT,i,5,MPI_COMM_WORLD,&mpistat);
    }
    if(flag) printf("everyone is locally sorted\n");
    else {
      printf("at least one failed to sort himself locally\n");
      exit(1);
    }
    //from now on flag has a new interpretetion
    for(i=0;i<num_tasks-1;i++){
      if(maximum[i] > minimum[i+1]) flag = 0;   
    }
    //if flag is still 1 at this point, test has been passed
    if(flag) printf("parallel bitonic sort was a success\n");
    else printf("bitonic sort has failed\n");
    
    
  } 
  else{
    //tag = 3 means local sorting flag
    rc = MPI_Send(&flag,1,MPI_INT,0,3,MPI_COMM_WORLD);
    //tag = 4 means minimum
    rc = MPI_Send(&min,1,MPI_INT,0,4,MPI_COMM_WORLD);
    //tag = 5 means maximum
    rc = MPI_Send(&max,1,MPI_INT,0,5,MPI_COMM_WORLD);
     
    
  } 
  
}
