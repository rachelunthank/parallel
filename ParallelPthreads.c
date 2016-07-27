#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define DIMENSION_OF_ARRAY 5

int THREAD_NUM;
int count; 
double PRECISION;
int n = DIMENSION_OF_ARRAY;
double arr[DIMENSION_OF_ARRAY][DIMENSION_OF_ARRAY] = {{1.164, 1.0712, 1.3918, 1.2118, 1.9153},{1.1923, 1.1908, 1.6614, 1.3763, 1.5957},{1.3428, 1.4676, 1.7797, 1.0386, 1.8736},{1.9985, 1.5087, 1.5195, 1.2975, 1.7967},{1.6812, 1.5034, 1.0552, 1.0057, 1.8921}};
double temparr[DIMENSION_OF_ARRAY][DIMENSION_OF_ARRAY];
pthread_mutex_t lock; 


struct arg_struct {
    int CALCULATIONS_PER_THREAD;
    int CALCULATIONS_PER_THREAD_REMAINDER;
    int INNER_ARRAY_SIZE; 
};

int thread_ID() {
  pthread_mutex_lock(&lock); 
  count++;
  pthread_mutex_unlock(&lock);
  return count; 
}

void * average_numbers(void *arguments){

  struct arg_struct *args = arguments;
  int NUMS_PER_THREAD = args->CALCULATIONS_PER_THREAD;
  int NUMS_REMAINDER = args->CALCULATIONS_PER_THREAD_REMAINDER;
  int INNER_ARRAY_SIZE = args->INNER_ARRAY_SIZE;

  int ID = thread_ID();

  int START_NUM = 0 + ((ID-1)*NUMS_PER_THREAD);
  int END_NUM = (ID*NUMS_PER_THREAD)-1;
  if (ID == THREAD_NUM){
    END_NUM = END_NUM+ NUMS_REMAINDER;
  }

  //This method finds the surrounding numbers and averages them, then replaces the number in the matrix with the average
  int i;
  for(i = START_NUM; i <= END_NUM; i++) {
    int x = (i%INNER_ARRAY_SIZE)+1;
    int y = ((i-(i%INNER_ARRAY_SIZE))/INNER_ARRAY_SIZE)+1;
    //above
    double a = arr[y-1][x];
    //right
    double b = arr[y][x+1];
    //below
    double c = arr[y+1][x];
    //left
    double d = arr[y][x-1];

    double average = (a+b+c+d)/4;
    temparr[y][x] = average;

  }

  pthread_exit (NULL);
	
}

void matrix_work(int n, double arr[][n]){

  //split numbers in matrix into number of threads
  int INNER_ARRAY_SIZE = n-2;
  int TOTAL_NUMS = (INNER_ARRAY_SIZE)*(INNER_ARRAY_SIZE);

  if(THREAD_NUM > TOTAL_NUMS) {
    THREAD_NUM = TOTAL_NUMS; 
  }
  int NUMS_PER_THREAD = (int)floor(TOTAL_NUMS / THREAD_NUM);
  int NUMS_REMAINDER = (TOTAL_NUMS % THREAD_NUM);

  struct arg_struct args;
  args.CALCULATIONS_PER_THREAD = NUMS_PER_THREAD;
  args.CALCULATIONS_PER_THREAD_REMAINDER = NUMS_REMAINDER;
  args.INNER_ARRAY_SIZE = INNER_ARRAY_SIZE;
  
  //create threads
  pthread_t threads [THREAD_NUM];
  count = 0;
  int i;
  for (i = 0; i < THREAD_NUM; i++) {
    pthread_create(&(threads[i]), NULL, average_numbers, (void *)&args);
  }
  
  //join threads
  int j;
  for (j = 0; j < THREAD_NUM; j++)
  {
    pthread_join(threads[j], NULL);

  }

}

void print_array(int n, double arr[][n]){
  //prints the array to command land
  int i;
  int j;
	for(i = 0; i < n; i++) {
	for(j = 0; j < n; j++) {
        printf("%f ", arr[i][j]);
    }
    printf("\n");
   }
}

int get_diff(){
  //prints the array to command land
  int i;
  int j;
  int flag = 0;
  for(i = 0; i < n; i++) {
    for(j = 0; j < n; j++) {
        double diff = fabs(arr[i][j] - temparr[i][j]);
        if (diff > PRECISION){flag=1;}

    }
  }
  return flag;
}

int main(int argc, char *argv[])
{
   //number of threads
   THREAD_NUM = atoi(argv[1]);
   //precision to work to
   PRECISION = atof(argv[2]);

   pthread_mutex_init(&lock, NULL); 

   count = 0; 
   printf("Original Array\n");
   print_array(n, arr);

   memcpy(temparr, arr, sizeof(arr));
   matrix_work(n, arr);

   while(get_diff()!=0){
   	//printf("change\n");
   	//print_array(n, arr);
    memcpy(arr, temparr, sizeof(arr));
   	matrix_work(n, arr);
   }

   printf("Array after relaxation technique applied\n");
   print_array(n, arr);

   pthread_mutex_destroy(&lock);

   return 0;
}