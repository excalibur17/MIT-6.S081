#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

const int R=0, W=1;

void redirect(int k, int* p){
	close(k); // release fd k, so that dup generate a new fd=k
	dup(p[k]); // redirect, generate a new fd which is the smallest unused number, pointing to the same file with p[k]
	close(p[0]); // close the original pipe fd
	close(p[1]);
	//exit(0); // Attention! exit() will terminate the whole process, not only th function itself.
}

void base_pipe(){
  int a, p[2];
  if(read(R, &a, sizeof(a))>0){ // read from R
  	pipe(p); // create a new pipe
		if(fork()!=0){ // parent
			printf("prime %d\n", a); // write to 1, note that 1 is not redirected yes and it's still standard output here
			redirect(W, p); // alought redirect is not necessary, close unused fd will save resources, also close(p[0]) and close(p[1]) will work the same
			
			int b;
			while(read(R, &b, sizeof(b))>0){
				if(b%a!=0){
				  write(W, &b, sizeof(b));  // write to W
				}
			}
			close(W); // without this shell cannot stop exec, cause the child's read will always wait for its write end, until the write end closes.
			wait(0); // wait for child's exit, it's a good habit
		}
		else{ // child
			redirect(R, p); // actually child did nothing but redirect R and call itself
			base_pipe();
		}
	}
  exit(0);
}

int main(int argc, char *argv[]){
   int i=0, p[2];
   pipe(p);
   for(i=2; i<=35; i++){
     if(write(p[1], &i, sizeof(i))==-1){
       printf("error write");
     }
   }
   //close(p[1]); // close write end if no data will write anymore, otherwise read from read end will always wait and won't exit. Also p[1] will be closed in redirect
   redirect(R, p); // redirect R to the read end of p
   base_pipe();
   exit(0);
}

// Another way of main that uses fork, alought 
/*int main(int argc, char *argv[])*/
/*{*/
/*    int pd[2];*/

/*    pipe(pd);*/

/*    if (fork() == 0)    //child*/
/*    {*/
/*        redirect(R, pd); // Attention that 0 is redirected, but 1 is not, 1 is still standard output*/
/*        base_pipe();*/
/*    }*/
/*    else    // parent*/
/*    {*/
/*        redirect(W, pd); // attention that the redirection of 1 will not affect child's fd=1*/
/*        for (int i = 2; i < 36; i++)*/
/*        {*/
/*            write(W, &i, sizeof(i));*/
/*        }*/
/*        // close the write side of pipe, cause read returns 0 and then read will not infinitely wait*/
/*        close(W);*/
/*        // the main primes process should wait until all the other primes process exited*/
/*        wait(0);*/
/*        exit(0);*/
/*    }*/
/*    return 0;*/
/*}*/
