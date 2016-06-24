<h1> Distributed Bitonic Sort using MPI </h1>

This is a simple, yet fast implementation of the bitonic sort algorithm using MPI-C.
Its advantage against other algorithms is its distributed nature, namely that no Node of the cluster 
system assumed ever holds more than a slice of the sorted table. As a result tables of arbitrary size can be 
sorted, since the RAM resources of the participating machines no longer poses a limitation.

<h2> Usage </h2>

The code can be compiled using:

mpicc Mpi_bitonic.c -o Mpi_bitonic
mpirun -np <b>P</b> Mpi_bitonic <b>N</b>

where P is the number of processes and N is the size of the problem.

Alternatively tha user can launch a spawner which accepts the input arguments via 
the standard input at runtime by running:

mpicc Spawner.c -o Spawner
mpirun Spawner
