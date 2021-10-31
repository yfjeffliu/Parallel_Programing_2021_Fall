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
   int    myid, numprocs;

   double startwtime = 0.0, endwtime;
   int    namelen;
   char   processor_name[MPI_MAX_PROCESSOR_NAME];
   double i;               /* loop variable (32 bits) */
   long long int count = 0;
   long long int receive_count = 0;
   long long int n=30000;

   MPI_Init(&argc,&argv);
   MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
   MPI_Comm_rank(MPI_COMM_WORLD,&myid);
   MPI_Get_processor_name(processor_name,&namelen);
   fprintf(stdout,"Process %d of %d is on %s\n",
   myid, numprocs, processor_name);
   fflush(stdout);

   if(myid == 0)
      startwtime = MPI_Wtime(); 
   for(i = myid ; i <= n;i+=numprocs){
      double j;
      for (j=0;j<=n;j+= 1 ){
         //printf("this is i %f and j %f\n",i,j);
         if(((i*i)/(n*n) + (j*j)/(n*n)) <= 1){
            count +=1;
            //printf("add\n");
         }else{
            ;
            break;
         }
      }
   }
   int process = numprocs;
   while(process != 1){
      if (process%2==0){
         if (myid < process/2){
            MPI_Recv(&receive_count,1,MPI_LONG_LONG_INT,myid+process/2,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            count += receive_count;
         }else{
            MPI_Send(&count,1,MPI_LONG_LONG_INT,myid-process/2,0,MPI_COMM_WORLD);
         }
         process /= 2;
      }else{
         if (myid != process/2){
            if (myid < process/2){
               MPI_Recv(&receive_count,1,MPI_LONG_LONG_INT,myid+(process+1)/2,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
               count += receive_count;
            }else{
               MPI_Send(&count,1,MPI_LONG_LONG_INT,myid-(process+1)/2,0,MPI_COMM_WORLD);
            }
         }
         process /= 2;
         process ++;
      }

   
     // printf("now process is %d\n",process);
   }

   if (myid == 0){
      
      double pi = 4 * count/((double)n*n);
      printf ("pi is  %.10f \n", pi);
      endwtime = MPI_Wtime();
      printf("wall clock time = %f\n", endwtime-startwtime);
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



