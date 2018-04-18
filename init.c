#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_PIPE 10
void exec(char *args[][128], int fildes[2], int now, int pipes);
int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[MAX_PIPE][128];

    while (1) {
        int pipes = 0;          //管道数目
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
        /* 清除前导空格 */
        for (i = 0; cmd[i] == ' ' || cmd[i] == '\t'; i++)
            ;

        /* 拆解命令行 */
        args[pipes][0] = &cmd[i];
        for (i = 0; *args[pipes][i]; i++) {
            for (args[pipes][i+1] = args[pipes][i] + 1; *args[pipes][i+1]; args[pipes][i+1]++) {
                if (*args[pipes][i+1] == ' ' || *args[pipes][i+1] == '\t') {
                    *args[pipes][i+1] = '\0';

                    /* 消除多余空格 */
                    do {
                    args[pipes][i+1]++;
                    } while (*args[pipes][i+1] == ' ' || *args[pipes][i+1] == '\t');

                    break;
                }
                if (*args[pipes][i+1] == '|') {
                    break;
                }
            }
            if (*args[pipes][i+1] == '|') {
                *args[pipes][i+1] = '\0';
                args[pipes + 1][0] = args[pipes][i+1] + 1;
                args[pipes][i+1] = NULL;

                pipes++;
                while (*args[pipes][0] == ' ' || *args[pipes][0] == '\t')
                    args[pipes][0]++;
                i = -1;
            }
        }
        args[pipes][i] = NULL;

        /* 没有输入命令 */
        if (!args[0][0])
            continue;

        /* 内建命令 */
        if (pipes == 0) {
            if (strcmp(args[pipes][0], "cd") == 0) {
                if (args[pipes][1]) {
                    if (chdir(args[pipes][1]) == -1) {
                        /* chdir失败 */
                        perror(NULL);
                    }
                }
                continue;
            }
            if (strcmp(args[pipes][0], "pwd") == 0) {
                char wd[4096];
                if (getcwd(wd, 4096) == NULL) {
                    /* getcwd失败 */
                    perror(NULL);
                }
                else
                    puts(wd);
                continue;
            }
            if (strcmp(args[pipes][0], "export") == 0) {
                int j;
                for (i = 1; args[pipes][i] != NULL; i++) {
                    for (j = 0; args[pipes][i][j]; j++) {
                        if (args[pipes][i][j] == '=') {
                            args[pipes][i][j] = '\0';
                            if (setenv(args[pipes][i], &args[pipes][i][j + 1], 1) == -1) {
                                /* setenv失败 */
                                perror(NULL);
                            }
                            break;
                        }
                    }
                }
                continue;
            }
            if (strcmp(args[pipes][0], "unset") == 0) {
                for (i = 1; args[pipes][i] != NULL; i++) {
                    if (unsetenv(args[pipes][i]) == -1) {
                        perror(NULL);
                    }
                }
                continue;
            }
            if (strcmp(args[pipes][0], "exit") == 0)
                return 0;
        }

        /*外部命令*/
        int fildes[2];
        int status;

        status = pipe(fildes);
        if (status == -1 ) {
            /* 创建管道失败 */
            perror(NULL);
            continue;
        }

        exec(args, fildes, 0, pipes);
    }
}

/*
* args为命令列表
* fildes为与前一个进程共用的管道
* now为当前命令在args中的索引
* pipes为管道数
*/
void exec(char *args[][128], int fildes[2], int now, int pipes)
{
    if (pipes == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程 */
            if (execvp(args[pipes][0], args[pipes]) == -1) {
                /* execvp失败 */
                perror(NULL);
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
        else if (pid == -1) {
            /* fork失败 */
            perror(NULL);
            return;
        }
        /* 父进程 */
        wait(NULL);
        return;
    }
    else if (now == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            close(fildes[0]);                       /* 读入端不用 */
            dup2(fildes[1], STDOUT_FILENO);
            close(fildes[1]);                       /* 关闭副本 */
            if (execvp(args[now][0], args[now]) == -1) {
                /* execvp失败 */
                perror(NULL);
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
        else if (pid == -1) {
            /* fork失败 */
            perror(NULL);
            close(fildes[1]);
            close(fildes[0]);
            return;
        }
         
        exec(args, fildes, now + 1, pipes);

        wait(NULL);
        return;
    }
    else if (now < pipes) {
        int fildes_other[2];
        int status;
            
        status = pipe(fildes_other);
        if (status == -1 ) {
            /* 创建管道失败 */
            perror(NULL);
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(fildes[1]);                       /* 前一个管道写入端不用 */
            dup2(fildes[0], STDIN_FILENO);
            close(fildes[0]);

            close(fildes_other[0]);                 /* 现在的管道读入端不用 */
            dup2(fildes_other[1], STDOUT_FILENO);
            close(fildes_other[1]);

            if (execvp(*args[now], args[now]) == -1) {
                /* execvp失败 */
                perror(NULL);
                exit(EXIT_FAILURE);
            }

            exit(EXIT_SUCCESS);
        }
        else if (pid == -1) {
            /* fork失败 */
            perror(NULL);
            close(fildes[1]);
            close(fildes[0]);
            close(fildes_other[0]);
            close(fildes_other[1]);
            return;
        }
        

        close(fildes[0]);                           /* 后面进程不需前一个管道 */
        close(fildes[1]);

        exec(args, fildes_other, now + 1, pipes);

        wait(NULL);
        return;
    }
    else if (now == pipes) {
        pid_t pid = fork();
        if (pid == 0) {
            close(fildes[1]);                       /* 写入端不用 */
            dup2(fildes[0], STDIN_FILENO);
            close(fildes[0]);
            if (execvp(args[now][0], args[now]) == -1) {
                /* execvp失败 */
                perror(NULL);
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
        else if (pid == -1) {
            /* fork失败 */
            perror(NULL);
            close(fildes[1]);
            close(fildes[0]);
            return;
        }
        close(fildes[1]);
        close(fildes[0]);
        wait(NULL);
        return;
    }
}