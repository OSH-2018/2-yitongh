#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_PIPE 10
#define MAX_REDIRECT 10
#define MAX_BUFFER 1024

enum Redirect_flag
{
    STDIN,
    STDOUT,
    STDERR,
    STDIN_ADD,
    STDOUT_ADD,
    STDERR_ADD
};

struct Redirect
{
    enum Redirect_flag type[MAX_REDIRECT];      //重定向，类型
    char *file[MAX_REDIRECT];        //重定向，文件位置
    int now; 
};

void exec(char *args[][128], int fildes[2], int now, int pipes);

int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[MAX_PIPE][128];

    while (1) {
        int pipes = 0;          //管道数目
        struct Redirect redirect_out, redirect_in, redirect_err;
        redirect_out.now = -1;
        redirect_in.now = -1;
        redirect_err.now = -1;

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
                
                /* 管道与重定向 */
                if (*args[pipes][i+1] == '|' || *args[pipes][i + 1] == '>' || *args[pipes][i + 1] == '<') {
                    break;
                }
            }
            /* 重定向 */
            if (*args[pipes][i + 1] == '1' || *args[pipes][i + 1] == '2'
                || *args[pipes][i + 1] == '>' || *args[pipes][i + 1] == '<') {
                if (*args[pipes][i + 1] == '<') {
                    redirect_in.now++;
                    redirect_in.type[redirect_in.now] = STDIN; 
                    *args[pipes][i + 1] = '\0';
                    /* 追加重定向 */
                    if (*(args[pipes][i + 1] + 1) == '<') {
                        args[pipes][i + 1]++;
                        redirect_in.type[redirect_in.now] = STDIN_ADD;
                    }
                    /* 消除多余空格 */
                    do {
                        args[pipes][i + 1]++;
                    } while (*args[pipes][i + 1] == ' ' || *args[pipes][i + 1] == '\t');

                    redirect_in.file[redirect_in.now] = args[pipes][i + 1];

                    i++;
                    args[pipes][i + 1] = args[pipes][i];
                    args[pipes][i] = NULL;
                }
                else if (*args[pipes][i + 1] == '>') {
                    redirect_out.now++;
                    redirect_out.type[redirect_out.now] = STDOUT;
                    *args[pipes][i + 1] = '\0';
                    /* 追加重定向 */
                    if (*(args[pipes][i + 1] + 1) == '>') {
                        args[pipes][i + 1]++;
                        redirect_out.type[redirect_out.now] = STDOUT_ADD;
                    }
                    /* 消除多余空格 */
                    do {
                        args[pipes][i + 1]++;
                    } while (*args[pipes][i + 1] == ' ' || *args[pipes][i + 1] == '\t');

                    redirect_out.file[redirect_out.now] = args[pipes][i + 1];

                    i++;
                    args[pipes][i + 1] = args[pipes][i];
                    args[pipes][i] = NULL;
                }
                else if (*(args[pipes][i + 1] + 1) == '>') {
                    if (*args[pipes][i + 1] == '1') {
                        redirect_out.now++;
                        redirect_out.type[redirect_out.now] = STDOUT;
                    }
                    else {
                        redirect_err.now++;
                        redirect_err.type[redirect_err.now] = STDOUT;
                    }
                    *args[pipes][i + 1] = '\0';
                    /* 追加重定向 */
                    if (*(args[pipes][i + 1] + 2) == '>') {
                        args[pipes][i + 1]++;
                        if (*args[pipes][i + 1] == '1')
                            redirect_out.type[redirect_out.now] = STDOUT;
                        else
                            redirect_err.type[redirect_err.now] = STDOUT;
                    }
                    args[pipes][i + 1]++;

                    /* 消除多余空格 */
                    do {
                        args[pipes][i + 1]++;
                    } while (*args[pipes][i + 1] == ' ' || *args[pipes][i + 1] == '\t');

                    if (*args[pipes][i + 1] == '1') {
                        redirect_out.file[redirect_out.now] = args[pipes][i + 1];
                    }
                    else {
                        redirect_err.file[redirect_err.now] = args[pipes][i + 1];
                    }
                    i++;
                    args[pipes][i + 1] = args[pipes][i];
                    args[pipes][i] = NULL;
                }
            }
            /* 管道 */
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

        
        if (pipes == 0) {
            /* 内建命令 */
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

            /* 外部命令 */
            int fildes[2];
            int status;

            status = pipe(fildes);
            if (status == -1) {
                /* 创建管道失败 */
                perror(NULL);
                continue;
            }
            pid_t pid = fork();
            if (pid == 0) {
                /* 子进程 */
                int fd_err[MAX_REDIRECT];
                int fd_out[MAX_REDIRECT];
                char buffer[MAX_BUFFER];
                ssize_t num_read;
                mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;            //文件拥有者有读写权限，组成员和其他人只有读权限

                close(fildes[1]);
                if (redirect_in.now != -1) {
                    dup2(fildes[0], STDIN_FILENO);
                }    
                close(fildes[0]);

                if (redirect_out.now != -1) {
                    for (i = 0; i <= redirect_out.now; i++) {
                        if (redirect_out.type[i] == STDOUT)
                            fd_out[i] = open(redirect_out.file[i], O_WRONLY | O_CREAT | O_TRUNC, mode);
                        else
                            fd_out[i] = open(redirect_out.file[i], O_WRONLY | O_CREAT | O_APPEND, mode);
                    }
                    dup2(fd_out[0], STDOUT_FILENO);
                    close(fd_out[0]);
                }
                if (redirect_err.now != -1) {
                    for (i = 0; i <= redirect_err.now; i++) {
                        if (redirect_err.type[i] == STDOUT)
                            fd_err[i] = open(redirect_err.file[i], O_WRONLY | O_CREAT | O_TRUNC, mode);
                        else
                            fd_err[i] = open(redirect_err.file[i], O_WRONLY | O_CREAT | O_APPEND, mode);
                    }
                    dup2(fd_err[0], STDERR_FILENO);
                    close(fd_err[0]);
                }
                if (execvp(args[pipes][0], args[pipes]) == -1) {
                    /* execvp失败 */
                    perror(NULL);
                    exit(EXIT_FAILURE);
                }
                for (i = 1; i <= redirect_out.now; i++) {
                    lseek(fd_out[0], 0, SEEK_SET);
                    while ((num_read = read(fd_out[0], buffer, MAX_BUFFER)) != 0)
                        write(fd_out[i], buffer, num_read);
                    close(fd_out[i]);
                }
                for (i = 1; i <= redirect_err.now; i++) {
                    lseek(fd_err[0], 0, SEEK_SET);
                    while ((num_read = read(fd_err[0], buffer, MAX_BUFFER)) != 0)
                        write(fd_err[i], buffer, num_read);
                    close(fd_err[i]);
                }
                if (redirect_out.now != -1)
                    close(fd_out[0]);
                if (redirect_err.now != -1)
                    close(fd_err[0]);
                exit(EXIT_SUCCESS);
            }
            else if (pid == -1) {
                /* fork失败 */
                perror(NULL);
                continue;
            }
            /* 父进程 */
            close(fildes[0]);
            char template[] = "./XXXXXX";
            int tmp_fd;
            if (redirect_in.now != -1) {
                tmp_fd = mkstemp(template);
                if (tmp_fd == -1) {
                    perror(NULL);
                    exit(0);
                }
                unlink(template);
            }
            char buffer[MAX_BUFFER];
            ssize_t num_read;
            for (i = 0; i <= redirect_in.now; i++) {
                int fd;
                if (redirect_in.type[i] == STDIN) {
                    fd = open(redirect_in.file[i], O_RDONLY);
                    if (fd == -1) {
                        perror(NULL);
                        exit(EXIT_FAILURE);
                    }
                    while ((num_read = read(fd, buffer, MAX_BUFFER)) != 0)
                        write(tmp_fd, buffer, num_read);
                    close(fd);
                }
                else {
                    while (1) {
                        num_read = read(STDIN_FILENO, buffer, MAX_BUFFER);
                        if (num_read == -1) {
                            perror(NULL);
                            break;
                        }
                        buffer[num_read - 1] = '\0';
                        if (strcmp(buffer, redirect_in.file[i]) == 0)
                            break;
                        buffer[num_read - 1] = '\n';
                        write(tmp_fd, buffer, num_read);
                    }
                }
            }
            if (redirect_in.now != -1) {
                lseek(tmp_fd, 0, SEEK_SET);
                while ((num_read = read(tmp_fd, buffer, MAX_BUFFER)) != 0)
                    write(fildes[1], buffer, num_read);
                close(tmp_fd);
            }
            
            close(fildes[1]);
            wait(NULL);
            continue;
        }

        /* 管道命令 */
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
    if (now == 0) {
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