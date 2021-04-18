#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
	int child2parent[2], parent2child[2];
	char c;
	pipe(child2parent); // two pipes, eventually there will be 8 file descriptors in parent and child pointing to the two pipes.
	pipe(parent2child);
	
	if(fork()==0){  // child
		close(parent2child[1]); // close useless file descriptors
		close(child2parent[0]);
		
		if(read(parent2child[0], &c, 1)<=0){ // read from parent's pipe
			fprintf(2, "read error in child.\n");
		}
		fprintf(1, "%d: received ping\n", getpid());
		write(child2parent[1], &c, 1); // write to child's pipe
		close(child2parent[1]); // close file descriptors
		close(parent2child[0]);
	}
	else{ // parent
		close(parent2child[0]);
		close(child2parent[1]);
		
		write(parent2child[1], &c, 1); // write to parent's pipe
		close(parent2child[1]); // this is ok, as there are data available in parent2child, read from parent2child[0] will not return 0
		
		if(read(child2parent[0], &c, 1)<=0){ // read from child's pipe, if no data available, will wait other fds that pointing to the write end of this pipe to write, or return 0 when all fds pointing to the write end closed.
			fprintf(2, "write error in parent.\n");
		}
		fprintf(1, "%d: received pong\n", getpid());
		
		close(child2parent[0]);
	}
	//wait(0); // no need for wait
	exit(0);
}

/* The following is my original code using only one single pipe*/
/* This part requires a pair of pipes each for one direction, so use a single pipe is not allowed.*/

/*int main(int argc, char *argv[]){*/
/*	int p[2], buf[20];*/
/*	pipe(p);*/

/*	int pid=fork();*/
/*	//fprintf(1, "pid=%d", pid);*/
/*	if(pid==0){*/
/*		//fprintf(1, "in child"); //evn though no wait in parent, still output "in child"*/
/*		int c=read(p[0], buf, 6); //but this cannot be exec, maybe no data in pipe*/
/*		//fprintf(1, "c=%d\n", c);  //c=6*/
/*		if(c<=0){*/
/*			fprintf(2, "read error");*/
/*		}*/
/*		int child_pid = getpid();*/
/*		fprintf(1, "%d: received ping\n", child_pid);*/
/*		if(write(p[1], "hello", 6)==-1){*/
/*			fprintf(1, "error in child write\n");*/
/*		}*/
/*		close(p[0]);*/
/*		close(p[1]);*/
/*	}*/
/*	else{*/
/*		if(write(p[1], "hello", 6)==-1){*/
/*			fprintf(2,"error in write");*/
/*		}*/
/*		wait(0); //wait for return of child, otherwise read after write, resulting no data in pipe.*/
/*		read(p[0], buf, 6);*/
/*		//fprintf(1, "buf=%s", buf); //buf is "hello"*/
/*		int pid = getpid();*/
/*		fprintf(1, "%d: received pong\n", pid);*/
/*		close(p[1]);*/
/*		close(p[0]);*/
/*	}*/
/*	exit(0);*/
/*}*/
