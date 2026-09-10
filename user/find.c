#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"
#include "user/regex.h"

int find(char *path, char *name, char **ap, char **ape) {
  /*printf("DEBUG: find: looking for %s in %s\n", name, path); */

  int fd;
  char buf[512], *p;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return -1;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    return -1;
  }

  /*printf("DEBUG: path: %s\n st.type: %d\n", path, st.type); */

  switch(st.type) {
    case T_DEVICE:
    case T_FILE:
      fprintf(2, "pattern: find name. name should be a dir\n");
      return -1;
    case T_DIR:

      /*printf("DEBUG: path is T_DIR\n"); */

      strcpy(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) // this means that the file is deleted
          continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        if (stat(buf, &st) < 0) {
          fprintf(2, "find: cannot stat %s\n", buf);
          return -1;
        }

        /*printf("DEBUG: finding %s in %s, current file is %s file's type is %d\n", name, path, p, st.type);*/

        switch(st.type) {
          case T_DIR:
            if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0) {
              ;
            } else {
              if (find(buf, name, ap, ape) < 0)
                return -1;
            }
            break;
          case T_FILE:
            printf("DEBUG: current name is %s, current file is %s\n", name, p);
            if (matchstr(name, p) == 1) {
              if (ap == 0) {
                printf("%s\n", buf);
              } else {
                int pid = fork();
                if (pid > 0) {
                  wait(0);
                } else if (pid == 0) {
                  char *eargv[MAXARG];
                  char **tmp = ap;
                  while (*tmp) {
                    eargv[tmp-ap] = *tmp;
                    tmp++;
                  }
                  eargv[tmp-ap] = buf;
                  exec(*eargv, eargv);
                  exit(1);
                } else {
                  fprintf(2, "find: cannot fork\n");
                  exit(1);
                }
              }
            } 
            break;
          case T_DEVICE:
            ;
            break;
        }
      }
  }
  close(fd);
  return 0;
}

int main(int argc, char *argv[]) {
  //MT: main() would executed after the preparation of find.c be done by kexec()
  //MT: kexec() would just put argc params into argv[0]...argv[argc] 
  //MT: after that, set argv[argc]
  char **p = 0;
  char **pe = 0;
  if (argc <= 2) {
    fprintf(2, "usage: [find dir name] or [find dir regex]\n");
    fprintf(2, "options: find dir name -exec cmd, this would execute cmd for each file found\n");
    exit(1);
  } else if (argc == 3) {
    if (find(argv[1], argv[2], p, pe) < 0)
      exit(1);
  } else if ((strcmp(argv[3], "-exec") == 0) && (argc >= 5)) {
    p = argv + 4;
    pe = argv + argc;
    if (find(argv[1], argv[2], p, pe) < 0)
      exit(1);
  } else {
    fprintf(2, "usage: find name\n");
    exit(1);
  } 
  exit(0);
}
