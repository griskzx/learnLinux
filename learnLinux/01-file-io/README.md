# 01 文件和文件描述符

今天只做这一件事。做完把答案写进同目录的 `notes.md`。

## 这个程序在干什么

`file_io.c` 做四步：创建 `sample.txt`，写进一句话，关掉，再打开把内容读回来。

在 F439 上读 Flash，是拿地址直接看那块内存。用户态程序碰不到磁盘地址。它调用 `open`，内核返回一个整数，叫文件描述符 `fd`。后面的 `read`、`write`、`close` 都带着这个整数。

这四个函数是系统调用。程序跑在用户态，执行到这里就进入内核，内核做完再回到程序。

`open` 的三个参数：

| 参数 | 这里的值 | 含义 |
| --- | --- | --- |
| 路径 | `"sample.txt"` | 当前目录下的文件 |
| 标志 | `O_CREAT \| O_WRONLY \| O_TRUNC` | 没有就创建，只写，已有内容则截断 |
| 权限 | `0644` | 创建时用：主人可读写，其他人只读 |

`sizeof text - 1` 是因为 C 字符串末尾有一个 `'\0'`，这个结尾不写进文件。

## 步骤

在本目录执行：

```bash
cd ~/projects/learnLinux/01-file-io
```

1. 读完 `file_io.c`。每个 `if (fd < 0)` 都是在问：内核拒绝了这次调用。
2. 编译并运行：

   ```bash
   make
   ./file_io
   cat sample.txt
   ```

3. 安装跟踪工具（当前环境还没有）。装完重新打开终端，或确认 `command -v strace` 有输出：

   ```bash
   sudo pacman -S strace
   strace -o trace.txt ./file_io
   ```

4. 在 `trace.txt` 里找 `openat`、`write`、`read`、`close`。只看这四类，其余行先跳过。
5. 回答 `notes.md` 里的五题。第 5 题要改代码，再编译运行，用结果核对你的预测。

`make clean` 会删掉可执行文件、`sample.txt` 和 `trace.txt`。
