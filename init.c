#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];

    while (1) {
        char **pipes = (char **)0;
        fflush(stdout);
        /* 提示符 */
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);
        /* 清理结尾的换行符 */
        int i;
        for (i = 0; cmd[i] != '\n'; i++)
            ;
        cmd[i] = '\0';
        /**/
        for (i = 0; cmd[i] == ' '; i++)
            ;
        /* 拆解命令行 */
        args[0] = &cmd[i];
        for (i = 0; *args[i]; i++) {
            for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++) {
                if (*args[i+1] == ' ') {
                    *args[i+1] = '\0';
                    /*消除多余空格*/
                    do {
                    args[i+1]++;
                    } while (*args[i+1] == ' ');

                    break;
                }
                if (*args[i+1] == '|') {
                    break;
                }
            }
            if (*args[i+1] == '|') {
                *args[i+1] = '\0';
                args[i+2] = args[i+1] + 1;
                args[i+1] = NULL;
                i++;
                while (*args[i+1] == ' ')
                    args[i+1]++;
                pipes = &args[i+1];
            }
        }
        args[i] = NULL;

        /* 没有输入命令 */
        if (!args[0])
            continue;

        /* 内建命令 */
        if (strcmp(args[0], "cd") == 0) {
            if (args[1])
                chdir(args[1]);
            continue;
        }
        if (strcmp(args[0], "pwd") == 0) {
            char wd[4096];
            puts(getcwd(wd, 4096));
            continue;
        }
        if (strcmp(args[0], "exit") == 0)
            return 0;

        if (pipes != (char **)0) {
            int fildes[2];
            int status;
            
            status = pipe(fildes);
            if (status == -1 ) {
               /* an error occurred */
               exit(0);
            }

            pid_t pid = fork();
            if (pid == 0) {
                close(fildes[1]);                       /* Write end is unused */
                dup2(fildes[0], 0);
                close(fildes[0]);
                if (execvp(*pipes, pipes) == -1) {
                    /* execvp失败 */
                    if (errno == EACCES) {
                        printf("error: you cannot perform this operation unless you are root.");
                    }
                    else if (errno == ENOEXEC) {
                        printf("bash: command not found: %s", args[0]);
                    }
                    exit(EXIT_FAILURE);
                }
                close(fildes[0]);                       /* Finished with pipe */
                exit(EXIT_SUCCESS);
            }

            pid = fork();
            if (pid == 0) {
                close(fildes[0]);                       /* Read end is unused */
                dup2(fildes[1], 1);
                close(fildes[1]);
                if (execvp(args[0], args) == -1) {
                    /* execvp失败 */
                    if (errno == EACCES) {
                        printf("error: you cannot perform this operation unless you are root.");
                    }
                    else if (errno == ENOEXEC) {
                        printf("bash: command not found: %s", args[0]);
                    }
                    exit(EXIT_FAILURE);
                }
                close(fildes[1]);                       /* Child will see EOF */
                exit(EXIT_SUCCESS);
            }
            close(fildes[1]);
            close(fildes[0]);
            wait(NULL);
            wait(NULL);
            continue;
        }
        /* 外部命令 */
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程 */
            if (execvp(args[0], args) == -1) {
                /* execvp失败 */
                if (errno == EACCES) {
                    printf("error: you cannot perform this operation unless you are root.");
                }
                else if (errno == ENOEXEC) {
                    printf("bash: command not found: %s", args[0]);
                }
                exit(EXIT_FAILURE);
            }
            
            return EXIT_SUCCESS;
        }
        /* 父进程 */
        wait(NULL);
    }
}
