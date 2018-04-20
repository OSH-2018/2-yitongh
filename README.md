# 实验二

## 添加的功能
- 多余空格处理，部分函数调用错误信息的输出，如`chdir`、`fork`、`execvp`等
- 数据重定向: 
```bash
ls > test
ls >> test
ls test no_such_file > test 2> test_err
cat << eof > test
cat < init.c  << eof > test
ls | wc > test2 >> test
```
- 命令行中`$变量`替换成相应的变量值, `~`替换成环境变量`HOME`的值