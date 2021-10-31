#include <iostream>
#include <mpi.h>
#include <algorithm>
using namespace std;
int *intlist_generator(int size);
void print_array(int *arr, int size);
int compute_partner(int phase,int rank,int totalrank);
void sort_merge(int *reuslt,int *keya,int *keyb,int size);
int main(int argc,char *argv[])
{
    int    myid, numprocs;              //own id,total process count
    double startwtime = 0.0, endwtime;
    int    namelen;
    char   processor_name[MPI_MAX_PROCESSOR_NAME];
    int n=0;
    // MPI init setting
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);
    
    MPI_Get_processor_name(processor_name,&namelen);
    fprintf(stdout,"Process %d of %d is on %s\n",myid, numprocs, processor_name);
    fflush(stdout);
    

    //get total number n
    if (myid == 0)
    {
        cout << "enter how many keys want to sort\n";
        cin >> n;
    }
    
    //set random seed
    srand(time(0) + myid + 1);
    //broadcast n to all nodes
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);


    int each_size = n/numprocs;                     //each node has how much number
    int *keys = intlist_generator(each_size);       //local list
    int *totalkeys = (int*)malloc(n*sizeof(int));
    

    sort(keys, keys+each_size);                     //sort local list

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Gather(keys, each_size, MPI_INT, totalkeys,each_size, MPI_INT,0,MPI_COMM_WORLD);
    if(myid == 0){
        cout << "print local list in each nodes " <<endl;
        for(int i = 0;i<numprocs;i++){
            cout << "list in node " << i << ": ";
            print_array(totalkeys+i*each_size, each_size);
        }
        
    }
    
    if(myid == 0){
            startwtime = MPI_Wtime(); 
    }
    int phase;
    int partner;
    int *reveive_keys = (int*)malloc(each_size*sizeof(int));
    int *double_keys = (int*)malloc(2*each_size*sizeof(int));
    MPI_Request request[2];
    MPI_Status status;
    for(phase=0;phase<numprocs;phase++){
        partner = compute_partner(phase,myid,numprocs);
        if(partner!= MPI_PROC_NULL){
            //MPI_Barrier(MPI_COMM_WORLD);
            MPI_Isend(keys,each_size,MPI_INT,partner,0,MPI_COMM_WORLD,&request[0]);
            MPI_Irecv(reveive_keys,each_size,MPI_INT,partner,0,MPI_COMM_WORLD,&request[1]);
            MPI_Wait(&request[0],&status);
            MPI_Wait(&request[1],&status);
            sort_merge(double_keys,keys,reveive_keys,each_size);
            if(myid < partner){
                for(int i = 0;i<each_size;i++){
                keys[i]=double_keys[i];
                }
            }else{
                for(int i = 0;i<each_size;i++){
                keys[i]=double_keys[i+each_size];
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Gather(keys, each_size, MPI_INT, totalkeys, each_size, MPI_INT, 0, MPI_COMM_WORLD);
    if(myid == 0){
        endwtime = MPI_Wtime();
        cout << "result : ";
        print_array(totalkeys, n);
        cout << "The execution time = "<< endwtime-startwtime <<endl ;
        
    }
    free(reveive_keys);
    free(keys);
    free(double_keys);
    MPI_Finalize();
    return 0;
}
void sort_merge(int *result,int *keya,int *keyb,int size)
{
    int posa=0;
    int posb=0;
    int postotal=0;
    while(posa != size || posb != size){
        if(posa == size){
            result[postotal]=keyb[posb];
            posb++;
        }else if(posb == size){
            result[postotal]=keya[posa];
            posa++;
        }else if(keya[posa]<=keyb[posb]){
            result[postotal] = keya[posa];
            posa++;
        }else{
            result[postotal] = keyb[posb];
            posb++;
        }
        postotal ++;
    }
}
int *intlist_generator(int size)
{
    int *list = (int *)malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++)
    {
        list[i] = rand()%1000000;
    }
    return list;
}
int compute_partner(int phase,int rank,int totalrank){
    int partner;
    if (phase%2==0){
        if(rank%2==0){
            partner = rank+1;
        }else{
            partner=rank-1;
        }
    }else{
        if(rank%2==0){
            partner = rank-1;
        }else{
            partner=rank+1;
        }
    }
    if((partner == -1) || (partner == totalrank)){
        partner = MPI_PROC_NULL;
    }
    return partner;
}
void print_array(int *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        cout << arr[i] << " ";
    }
    cout << endl;
}
