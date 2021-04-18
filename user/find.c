#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


char*  // made a small change from ls.fmtname, only remove the last spaces 
fmtname(char *path) // from right to left, find '/' in the path and stop, output the right side
{  // which means only output the pure filename, e.g., ./out.txt -> out.txt
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  *(buf+strlen(p)) = '\0'; // you must set the end of string because buf is static
  return buf;
}

void
find(char *path, char* str)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){ // get file info from fd
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE: // T_FILE=2, maybe this means the path is a file not a dir, and output this file's info
    if(strcmp(fmtname(path), str)==0){ // when path is a file path
        printf("%s\n", path);
    }
    break;

  case T_DIR: // T_DIR=1, means the path is a dir
    // printf("path=%s\n", path); // path is path from console input
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/'; // which gets buf='path/'
    while(read(fd, &de, sizeof(de)) == sizeof(de)){ // find all files in path
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ); // here buf='path/filename'
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){ // from new path buf get file info
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
    //   printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
      // you should skip . and ..
      if(strcmp(de.name, ".")!=0 && strcmp(de.name, "..")!=0 && st.type==T_DIR){
          find(buf, str); // recursively find
      }
      else if(strcmp(str, de.name)==0){
          printf("%s\n", buf);
      }
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[]){
    if(argc<3){
        fprintf(1, "not enough parameters.\n");
    }

    find(argv[1], argv[2]);
    exit(0);
}