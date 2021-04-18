#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
//#include "user/ulib.c"

int main(int argc, char *argv[]){
  if(argc<=1){ // command itself is also part of arguments
		fprintf(1, "No time specified!\n");
		exit(1);
	}
	//fprintf(1, argv[0]);  // sleep
	sleep(atoi(argv[1]));  // argv[0] is "sleep"
	exit(0);
}
