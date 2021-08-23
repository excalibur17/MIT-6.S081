#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]){
    int i;
    // printf("MAXARG=%d\n", MAXARG); // 32
    char *_argv[MAXARG];
    for(i=1; i<argc; i++){
        _argv[i-1]=argv[i];
    }
    char buf[1024];
    int stat;
    while(1){
        int buf_begin=0, buf_cur=0;
        int arg_idx = argc-1;
        char c;
        while((stat=read(0, &c, 1))>0){ //read one line
            if(c==' ' || c=='\n'){ // seperation of strings
                buf[buf_cur++]='\0'; //set end of string
                _argv[arg_idx++]=buf+buf_begin;
                // printf("%s\n", _argv[arg_idx-1]); // hello, too
                buf_begin=buf_cur;
                if(c=='\n'){
                    break;
                }
            }
            else{
                buf[buf_cur++]=c;
            }
        }
        if(stat==0){ // if no data in standard input
            exit(0);
        }
        _argv[arg_idx]=0; // NULL pointer indicate end of args
        if(fork()==0){
            exec(_argv[0], _argv);
        }
        else{
            wait(0);
        }
    }
    exit(0);
}