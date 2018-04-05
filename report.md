# 实验二 report

## 准备编译环境
1. 下载Ubuntu 16.04.2 Base Image(AMD64)镜像包
    ```
    # wget -O /opt/ubuntu-base-16.04.2-base-amd64.tar.gz http://mirrors.ustc.edu.cn/ubuntu-cdimage/ubuntu-base/releases/16.04/release/ubuntu-base-16.04.2-base-amd64.tar.gz
    ``` 
2. 解压镜像
    ```
    # mkdir -pv /opt/ubuntu
    # cd /opt/ubuntu
    # tar xpzf ../ubuntu-base-16.04.2-base-amd64.tar.gz
    ```
3. 切换根目录前的准备
    - 配置DNS
        ```
        cp -v /etc/resolv.conf /opt/ubuntu/etc/resolv.conf
        ```
    - 将官方源地址修改为科大开源镜像站
        ```
        # wget -O /opt/ubuntu/etc/apt/sources.list https://mirrors.ustc.edu.cn/repogen/conf/ubuntu-http-4-xenial
        ```
4. 安装编译环境
    - 改变根目录
        ```
        # chroot /opt/ubuntu /bin/bash
        ```
        -  以上命令会将原来的环境变量导入到新的根目录环境中，会出现诸如*command not found* , *perl: warning: Setting locale failed*等各种奇怪的问题。故改为下面的命令:
        ```
        # chroot /opt/ubuntu /usr/bin/env -i \
        HOME=/root TERM="$TERM" PS1='\u:\w\$ ' \
        PATH=/usr/sbin:/usr/bin:/sbin:/bin \
        /bin/bash
        ```
        - 给 env 命令传递 -i 选项会清除这个 chroot 切换进去的环境里所有变量。随后，只重新设定了 HOME、TERM、PS1 和 PATH 变量。TERM=$TERM 语句会设定 chroot 进入的环境里的 TERM 变量为进入前该变量同样的值。许多程序需要这个变量才能正常工作，比如 vim 和 less。参见[进入chroot环境](https://linux.cn/lfs/LFS-BOOK-7.7-systemd/chapter06/chroot.html)。
    - 更新仓库索引
        ```
        # apt update
        ```
    - 安装软件包
        ```
        # apt install -y build-essential libncurses5-dev libgimp2.0-dev bc curl cpio
        # apt install vim
        ```
    