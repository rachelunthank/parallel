#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <string.h>

#define send_data_tag 2001
#define return_data_tag 2002

#define DIMENSION_OF_ARRAY 1000

int THREAD_NUM;
double PRECISION;
int n = DIMENSION_OF_ARRAY;
double arr[DIMENSION_OF_ARRAY][DIMENSION_OF_ARRAY];
double temparr[DIMENSION_OF_ARRAY][DIMENSION_OF_ARRAY];


void print_array(int n, double arr[][n]){
  //loops through the array and prints to output
  int i, j;
   for(i = 0; i < n; i++) {
      for(j = 0; j < n; j++) {
         printf("%f ", arr[i][j]);
      }
      printf("\n");
   }
}

int get_diff(){
  //loops through array values and checks to see if the answer has been achieved.
  int i, j;
  int flag = 0;
  for(i = 0; i < n; i++) {
    for(j = 0; j < n; j++) {
      //gets absolute value for old array minus new array
      double diff = fabs(arr[i][j] - temparr[i][j]);
      //if the difference is bigger than the array, then set the flag to 1
      if (diff > PRECISION){flag=1;}
    }
  }
  return flag;
}

int get_flag(){
   int flag;
   int root_process = 0;
   MPI_Status status;
   //receive flag from root process whether program has finished & return to slave process
   MPI_Recv(&flag, 1, MPI_INT, root_process, send_data_tag, MPI_COMM_WORLD, &status);
   return flag;
}

void matrix_work(int num_procs, int INNER_ARRAY_SIZE, int NUMS_PER_THREAD, int NUMS_REMAINDER, int TOTAL_NUMS){
   
   int flag, an_id, ierr, length, i;
   MPI_Status status;
   //initialise array
   double localarray[TOTAL_NUMS];

   //get number of processors
   ierr = MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
   //set flag to 0
   flag=0;
   //for each slave process send flag and array

   for(an_id = 1; an_id < num_procs; an_id++) {
      ierr = MPI_Send(&flag, 1, MPI_INT, an_id, send_data_tag, MPI_COMM_WORLD);
      ierr = MPI_Send(&(arr[0][0]), n*n, MPI_DOUBLE, an_id, send_data_tag, MPI_COMM_WORLD);
   }

   // collect work from slave processes and goes through and put into the temparr array
   

   for(an_id = 1; an_id < num_procs; an_id++) {
      ierr = MPI_Recv(&length, 1, MPI_INT, an_id, return_data_tag, MPI_COMM_WORLD, &status);
      
      ierr = MPI_Recv(&localarray, length, MPI_DOUBLE, an_id, return_data_tag, MPI_COMM_WORLD, &status);


      int START_NUM = 0 + ((an_id-1)*NUMS_PER_THREAD);
      int END_NUM = (an_id*NUMS_PER_THREAD)-1;
      if (an_id == THREAD_NUM){
         END_NUM = END_NUM + NUMS_REMAINDER;
      }

      int count=0;

      for(i = START_NUM; i <= END_NUM; i++) {
         //get the x & y values for corresponding number in the matrix
         int x = (i%INNER_ARRAY_SIZE)+1;
         int y = ((i-(i%INNER_ARRAY_SIZE))/INNER_ARRAY_SIZE)+1;
         //get four surrounding numbers and average them
         temparr[y][x] = localarray[count];
         count++;
      }  
   }

}

void fill_array(){
	int b, c;
	for(b=0; b< n; b++){
	  for(c=0; c< n; c++){
	     if (b==0){
	        arr[b][c] = 1;
	     } else {
	        arr[b][c] = 0;
	     }
	  }
	}
}

int main(int argc, char *argv[]) 
{
   //precision to work to
   sscanf(argv[1], "%lf", &PRECISION);

   fill_array();

   long int sum, partial_sum;
   MPI_Status status;
   int ID, root_process, ierr, i, num_procs, an_id, sender, length, flag;

   ierr = MPI_Init(&argc, &argv);
   
   root_process = 0;
   
   /* find out MY process ID, and how many processes were started. */
   
   ierr = MPI_Comm_rank(MPI_COMM_WORLD, &ID);
   ierr = MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

   THREAD_NUM = num_procs-1;

   int INNER_ARRAY_SIZE = n-2;
   int TOTAL_NUMS = (INNER_ARRAY_SIZE)*(INNER_ARRAY_SIZE);
      
   //splits total amount of numbers in matrix into number of threads beng used, and finds the remainder
   int NUMS_PER_THREAD = (int)floor(TOTAL_NUMS / THREAD_NUM);
   int NUMS_REMAINDER = (TOTAL_NUMS % THREAD_NUM);

   if(ID == root_process) {

      // ROOT PROCESS

      //copies original array into a temporary array
      memcpy(temparr, arr, sizeof(arr));

      matrix_work(num_procs, INNER_ARRAY_SIZE, NUMS_PER_THREAD, NUMS_REMAINDER, TOTAL_NUMS);

      while(get_diff()!=0){
         memcpy(arr, temparr, sizeof(arr));
         matrix_work(num_procs, INNER_ARRAY_SIZE, NUMS_PER_THREAD, NUMS_REMAINDER, TOTAL_NUMS);
      }


      flag=1;
      for(an_id = 1; an_id < num_procs; an_id++) {
         ierr = MPI_Send(&flag, 1, MPI_INT, an_id, send_data_tag, MPI_COMM_WORLD);
      }

   }
   else {

      // SLAVE PROCESS

      //while program not done, calculate averages from range in array and send back to root process
      while(get_flag()==0){
         ierr = MPI_Recv(&(arr[0][0]), n*n, MPI_DOUBLE, root_process, send_data_tag, MPI_COMM_WORLD, &status);
      

         //Calculates the start and end numbers in the matrix for the thread to work on
         int START_NUM = 0 + ((ID-1)*NUMS_PER_THREAD);
         int END_NUM = (ID*NUMS_PER_THREAD)-1;

         if (ID == THREAD_NUM){
            END_NUM = END_NUM+ NUMS_REMAINDER;
         }
         
         int range = END_NUM - START_NUM;
         double localnums[range];

         //loops through numbers from start to end that need to be worked on and overwrites the temp array
         int i;
         int count = 0;
         for(i = START_NUM; i <= END_NUM; i++) {
            //get the x & y values for corresponding number in the matrix
            int x = (i%INNER_ARRAY_SIZE)+1;
            int y = ((i-(i%INNER_ARRAY_SIZE))/INNER_ARRAY_SIZE)+1;
            //get four surrounding numbers and average them
            //above
            double a = arr[y-1][x];
            //right
            double b = arr[y][x+1];
            //below
            double c = arr[y+1][x];
            //left
            double d = arr[y][x-1];

            double average = (a+b+c+d)/4;

            localnums[count] = average;
            count++;
         }

         //and finally, send array of new numbers back to the root process
         ierr = MPI_Send(&count, 1, MPI_INT, root_process, return_data_tag, MPI_COMM_WORLD);
         ierr = MPI_Send(&localnums, count, MPI_DOUBLE, root_process, return_data_tag, MPI_COMM_WORLD);
      }
   }

   ierr = MPI_Finalize();
}