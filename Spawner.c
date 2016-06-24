/* manager */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "mpi.h"

int main(int argc, char *argv[])
{
   int world_size, universe_size, *universe_sizep, flag;
   MPI_Comm everyone;           /* intercommunicator */
   char worker[] = "MPI_bitonic";
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &world_size);

   if (world_size != 1)    error("Top heavy with management");
   MPI_Attr_get(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &universe_sizep, &flag);

   printf("Provide P where 2^P is the number of processes\n");
   scanf("%d", &universe_size);
   universe_size = 1<<universe_size;


   if (universe_size == 1) error("No room to start workers");

   int temp = 0;
   printf("Please provide the total number of elements to be sorted \n");
   scanf("%d",&temp);
   char str[1];
   sprintf(str, "%d", temp);
   char* arg[] = {str,NULL};

   /*
    * Now spawn the workers. Note that there is a run-time determination
    * of what type of worker to spawn, and presumably this calculation must
    * be done at run time and cannot be calculated before starting
    * the program. If everything is known when the application is
    * first started, it is generally better to start them all at once
    * in a single MPI_COMM_WORLD.
    */

   MPI_Comm_spawn(worker, arg, universe_size-1,
             MPI_INFO_NULL, 0, MPI_COMM_SELF, &everyone,
             MPI_ERRCODES_IGNORE);
   /*
    * Parallel code here. The communicator "everyone" can be used
    * to communicate with the spawned processes, which have ranks 0,..
    * MPI_UNIVERSE_SIZE-1 in the remote group of the intercommunicator
    * "everyone".
    */
}