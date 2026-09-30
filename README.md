# Linux 学习

从用户态系统编程走到嵌入式 Linux。内核模块和 Rust for Linux 放在能自己启动一个小系统之后。

## 现在做这个

- [01 文件和文件描述符](learnLinux/01-file-io/README.md)

打开 `learnLinux/01-file-io`，读 `file_io.c`，编译运行，把五道题写进 `notes.md`。

## 后面再做

1. 进程、`fork`、管道
2. 用 QEMU 和 Buildroot 启动一个小系统，分清工具链、内核、设备树、根文件系统
3. 在板子的用户态访问串口、I2C、GPIO
4. 对着正在运行的内核写一个 C 的 hello 模块
5. 再看 Rust for Linux

[WSL 与内核源码的环境记录](notes/01-rust-for-linux-wsl-arch.md) 留到第 4 步再用。现在不编译 `rust-linux-lab` 里的内核。

## 目录

- `learnLinux/`：代码实验，每个实验一个子目录。
- `notes/`：环境记录。做完实验后的观察写在该实验目录的 `notes.md`。
- `rust-linux-lab/`：本地内核源码，已加入 Git 忽略规则。

不要把内核源码、编译产物或虚拟机镜像提交到本仓库。
