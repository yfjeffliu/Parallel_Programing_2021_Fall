/* circuitSatifiability.c solves the Circuit Satisfiability
 *  Problem using a brute-force sequential solution.
 *
 *   The particular circuit being tested is "wired" into the
 *   logic of function 'checkCircuit'. All combinations of
 *   inputs that satisfy the circuit are printed.
 *
 *   16-bit version by Michael J. Quinn, Sept 2002.
 *   Extended to 32 bits by Joel C. Adams, Sept 2013.
 */

#include <stdio.h>     // printf()
#include <limits.h>    // UINT_MAX
#include <mpi.h>
int checkCircuit (int, int);

int main (int argc, char *argv[]) {
   double i;               /* loop variable (32 bits) */
   long long int count = 0;
   long long int mycount = 0;
   int comm_sz; // number of proccesses
   int my_rank; // process rank
   double startTime = 0.0, totalTime = 0.0; 
   long long int n=300000;

   
   MPI_Init(NULL,NULL);
   MPI_Comm_size(MPI_COMM_WORLD,&comm_sz);
   MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);

   if(my_rank == 0)
      startTime = MPI_Wtime(); 
   for(i = my_rank ; i <= n;i+=comm_sz){
      double j;
      for (j=0;j<=n;j+= 1 ){
         //printf("this is i %f and j %f\n",i,j);
         if(((i*i)/(n*n) + (j*j)/(n*n)) <= 1){
            mycount +=1;
            //printf("add\n");
         }else{
            ;
            //break;
         }
      }
   }
   if (my_rank == 0 ){
      int q;
      count += mycount;
      for (q=1;q<comm_sz;q++)
      {
         MPI_Recv(&mycount,1,MPI_LONG_LONG_INT,q,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
         count += mycount;
      }
   }else{
      MPI_Send(&mycount,1,MPI_LONG_LONG_INT,0,0,MPI_COMM_WORLD);
  }

   if (my_rank == 0){
       totalTime = MPI_Wtime() - startTime; 
       printf("Process %d finished in time %f secs.\n", my_rank, totalTime); 
      double pi = 4 * count/((double)n*n);
      printf ("pi is  %.10f \n", pi);
      fflush (stdout);
   }
  
   MPI_Finalize();

   return 0;
}

/* EXTRACT_BIT is a macro that extracts the ith bit of number n.
 *
 * parameters: n, a number;
 *             i, the position of the bit we want to know.
 *
 * return: 1 if 'i'th bit of 'n' is 1; 0 otherwise 
 */



/* checkCircuit() checks the circuit for a given input.
 * parameters: id, the id of the process checking;
 *             bits, the (long) rep. of the input being checked.
 *
 * output: the binary rep. of bits if the circuit outputs 1
 * return: 1 if the circuit outputs 1; 0 otherwise.
 */

#define SIZE 16



