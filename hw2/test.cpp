#include <mpi.h>
#include <iostream>
using namespace std;
int main (int argc, char *argv[]) {
   int    myid, numprocs;

   double startwtime = 0.0, endwtime;
   int    namelen;
   char   processor_name[MPI_MAX_PROCESSOR_NAME];


   MPI_Init(&argc,&argv);
   MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
   MPI_Comm_rank(MPI_COMM_WORLD,&myid);
   MPI_Get_processor_name(processor_name,&namelen);
   fprintf(stdout,"Process %d of %d is on %s\n",
   myid, numprocs, processor_name);
   fflush(stdout);
  
   
    
    cout << 1 << endl;
    MPI_Barrier(MPI_COMM_WORLD);
    cout << 2 << endl;
  
    
    
   
    MPI_Finalize();
    return 0;
}