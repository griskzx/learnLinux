# Rust for Linux 入门：WSL 2、Arch 环境与内核配置

记录日期：2026-09-28。

## 学习背景与目标

已有 Rust 嵌入式开发经验，主要使用 STM32；学过操作系统课程，需要在实验中复习；能阅读基本 C 语法，宏、`goto` 等写法需要逐步补充。

本次目标是用 Rust 编写和运行 Linux 内核模块、驱动。已有的 `no_std`、寄存器、外设、中断和资源管理经验可以迁移，重点补充 Linux 的设备管理、执行上下文、内存与并发模型。

第一个完整实验的目标：在 QEMU 中启动自己编译的内核，加载、卸载 `rust_minimal`，观察初始化与退出日志。

## 当前进度

| 阶段 | 状态 |
| --- | --- |
| 确认 WSL 版本、架构、工具和内存 | 已完成 |
| 下载、解压 Linux 7.2.8 源码 | 已完成 |
| 执行 Rust 构建条件检查 | 已通过：`Rust is available!` |
| 配置 Rust 支持和最小示例模块 | 已完成并检查 |
| 编译内核和模块、检查产物 | 命令已准备，结果待验证 |
| 准备启动文件系统、在 QEMU 中启动 | 待进行 |
| 加载、卸载模块并查看日志 | 待进行 |

## 实验环境

| 项目 | 本次记录 |
| --- | --- |
| Windows 终端 | PowerShell 7.6.6 |
| WSL 发行版 | `archlinux`，WSL 2 |
| 当前运行的内核 | `6.18.33.2-microsoft-standard-WSL2` |
| CPU 架构 | `x86_64` |
| Rust 编译器 | `rustc 1.98.1`，通过 rustup 管理 |
| Cargo | `cargo 1.98.1` |
| Clang | `22.1.8` |
| bindgen | `0.73.2` |
| QEMU | `11.1.1` |
| WSL 可见内存 | 约 `7.6 GiB` |
| 实验内核源码版本 | 固定使用 `Linux 7.2.8` |

`rust-src`、LLVM/lld 和基本构建工具已经安装。本次没有另外安装一套 Rust 工具链。

### 检查命令

在 Windows PowerShell 中运行：

```powershell
wsl -l -v
```

发行版那一行的 `VERSION` 应为 `2`，这里表示 WSL 版本。

在 Arch 终端中运行：

```bash
uname -r
uname -m
rustc --version
cargo --version
command -v rustup clang bindgen qemu-system-x86_64
free -h
```

`uname -r` 查看当前运行的内核，`uname -m` 查看架构，`command -v` 查找已有工具的路径。

## 先建立三个概念

### 发行版与内核

Arch 提供用户空间的程序、包管理器和开发工具。在默认 WSL 2 环境中，这些程序运行在微软提供的 Linux 内核上。

本次实验中，WSL 用来编辑和编译；QEMU 用来启动独立的实验内核。

```text
Windows
└── WSL 2：Arch 开发环境
    ├── 编辑代码、编译 Linux 内核和 Rust 模块
    └── QEMU：虚拟 CPU、内存和设备
        └── 自己编译的 Linux 内核 + 实验用户空间
            └── 加载 rust_minimal.ko
```

模块需要和运行它的内核构建匹配。因此后面的 `rust_minimal.ko` 会在我们构建的 QEMU 实验内核中加载。

### 内核 Rust 与嵌入式 Rust

内核中的 Rust 使用 `no_std`，使用内核提供的日志、分配、锁和设备接口；构建过程由 Kbuild 管理。

遇到不熟悉的内核 API 时，应查所选内核版本的示例和文档。不同内核版本的 Rust 接口可能不同，不能直接混用教程中的代码。

### 从 STM32 迁移过来的重点

| 已有经验 | Linux 中要学习的内容 |
| --- | --- |
| 初始化外设、使用 HAL | 设备与驱动匹配、`probe`、资源释放 |
| 操作寄存器 | MMIO、地址映射、内核提供的访问接口 |
| 中断、临界区 | 进程上下文、中断上下文、多核同步 |
| 应用直接访问硬件 | 应用通过系统调用访问驱动管理的设备 |

优先复习用户态与内核态、系统调用、内核模块、虚拟地址和锁。读驱动时，特别关注“这里能否睡眠”和“这个对象是否可能被多个 CPU 同时访问”。

## 第一步：获取源码，检查 Rust 支持

以下实验命令都在 Arch 中执行。

### 下载与解压

```bash
mkdir -p ~/projects/rust-linux-lab
cd ~/projects/rust-linux-lab

curl -fL -o linux-7.2.8.tar.xz \
  https://cdn.kernel.org/pub/linux/kernel/v7.x/linux-7.2.8.tar.xz
```

下载成功后：

```bash
tar -xf linux-7.2.8.tar.xz
cd linux-7.2.8
```

源码中重点关注的目录：

```text
samples/rust/    Rust 示例模块和驱动
rust/kernel/    内核提供的 Rust 抽象与接口
Documentation/rust/    Rust 内核开发文档
```

### 构建条件检查

在源码目录中执行：

```bash
make LLVM=1 rustavailable
```

本次实际输出：

```text
Rust is available!
```

`LLVM=1` 选择 Clang/LLVM 工具链，`rustavailable` 检查 Rust 构建条件。通过检查意味着可以继续配置，完整编译的结果仍要在下一阶段验证。

## 第二步：配置 Rust 最小模块

### 生成默认配置

```bash
cd ~/projects/rust-linux-lab/linux-7.2.8
make LLVM=1 O=../build defconfig
```

- `defconfig`：生成当前架构的默认配置；本次基于 `x86_64_defconfig`。
- `O=../build`：将配置和编译产物放在旁边的 `build` 目录。`O` 是大写英文字母。
- `.config`：决定内核包含哪些功能。

目录布局：

```text
rust-linux-lab/
├── linux-7.2.8.tar.xz
├── linux-7.2.8/       内核源码
└── build/            配置和编译产物
    └── .config
```

### 开启 Rust、模块支持和示例

```bash
scripts/config --file ../build/.config \
  --enable RUST \
  --enable MODULES \
  --enable MODULE_UNLOAD \
  --enable SAMPLES \
  --enable SAMPLES_RUST \
  --module SAMPLE_RUST_MINIMAL
```

然后让配置系统处理选项之间的依赖：

```bash
make LLVM=1 O=../build olddefconfig
```

配置值的含义：

| 值 | 含义 |
| --- | --- |
| `y` | 编译进内核 |
| `m` | 编译成独立的 `.ko` 模块，可在支持模块的内核中加载 |
| `# CONFIG_... is not set` | 不启用该功能 |

`MODULE_UNLOAD=y` 允许卸载模块。将 `SAMPLE_RUST_MINIMAL` 设为 `m`，是为了在后续实验中观察加载、卸载的行为。

### 检查最终配置

```bash
rg '^CONFIG_(RUST|MODULES|MODULE_UNLOAD|SAMPLES|SAMPLES_RUST|SAMPLE_RUST_MINIMAL)=' \
  ../build/.config
```

本次已确认输出包含：

```text
CONFIG_RUST=y
CONFIG_MODULES=y
CONFIG_MODULE_UNLOAD=y
CONFIG_SAMPLES=y
CONFIG_SAMPLES_RUST=y
CONFIG_SAMPLE_RUST_MINIMAL=m
```

如果 `rg` 输出前面有 `301:` 等数字，那是文件中的行号。

## 问题复盘：数字 0 与字母 O

本次第一次执行时，将命令写成了：

```bash
make LLVM=1 0=../build defconfig
```

`make` 把 `0` 视为另一个变量，因此没有设置输出目录。配置和生成文件落到了源码目录，`../build/.config` 没有生成。

于是后续出现两类报错：

```text
../build/.config: No such file or directory
```

```text
The source tree is not clean, please run 'make mrproper'
```

第二类错误是因为切换到独立输出目录构建时，源码目录中仍有 `.config`、`include/config` 等生成文件。

此外，示例名曾写成 `SAMPLE_RUST_MINIMA`，缺少最后的 `L`。正确名称是 `SAMPLE_RUST_MINIMAL`。

### 本次修复方法

当时只有默认配置，没有需要保留的自定义配置，因此先在源码目录清理：

```bash
cd ~/projects/rust-linux-lab/linux-7.2.8
make LLVM=1 mrproper
```

`mrproper` 删除生成文件，包括源码目录里的 `.config`，保留内核源码。以后有需要保留的配置，应先保存再使用这条命令。

然后重新执行“第二步”的 `defconfig`、`scripts/config`、`olddefconfig` 和检查命令。本次修复后，六项配置均已确认正确。

出现报错时，先解决当前步骤，再继续下一条命令；后续报错可能只是前一步失败的结果。

## 第三步：编译与产物检查（待验证）

以下是下一步命令，当前记录中尚未确认编译成功。

在源码目录运行：

```bash
make LLVM=1 O=../build -j4
```

`-j4` 表示最多同时运行 4 个编译任务，先采用这个并行度。

构建日志中常见的标记：

| 标记 | 含义 |
| --- | --- |
| `CC` | 编译 C 代码 |
| `RUSTC` | 编译 Rust 代码 |
| `LD` | 链接 |

本次构建整个内核，因此会编译大量 C 文件。

命令成功退出后，检查产物：

```bash
ls -lh \
  ../build/arch/x86/boot/bzImage \
  ../build/samples/rust/rust_minimal.ko
```

| 文件 | 用途 |
| --- | --- |
| `bzImage` | 后面交给 QEMU 启动的内核镜像 |
| `rust_minimal.ko` | 要在实验内核里加载的 Rust 模块 |

如果失败，保留第一处 `error:` 附近内容和最后约 20 行日志，用于定位原因。

## 最小 Rust 模块：后续阅读重点

文件：`samples/rust/rust_minimal.rs`。

- `module!`：声明模块类型、名称、描述、许可证和参数。
- `kernel::Module::init`：模块初始化时调用，成功后返回模块对象。
- `Drop`：在这个可加载模块卸载时参与清理。
- `pr_info!`：写入内核日志。
- `KVec`、`GFP_KERNEL`：涉及内核内存分配，后续结合运行结果学习。

加载时预期看到 `Rust minimal sample (init)`，卸载时预期看到 `Rust minimal sample (exit)`。这些目前是源码中的预期行为，尚未通过加载实验验证。

## C 语法补充：goto 与宏

现阶段先能读懂资源申请、出错路径和资源释放。复杂写法可以结合具体驱动逐步学习。

### goto：统一清理资源

`goto` 跳到当前函数中的标签。以下是示意代码，`do_work` 表示某项操作；这里使用用户态 C 的 `malloc/free` 解释语法，内核代码使用自己的分配接口。

```c
int demo(void)
{
    void *buf = malloc(128);
    if (buf == NULL)
        return -1;

    int ret = do_work(buf);
    if (ret < 0)
        goto out_free;

    /* 成功后继续处理…… */
    ret = 0;

out_free:
    free(buf);
    return ret;
}
```

`out_free:` 是标签，`goto out_free` 直接跳到清理处。成功路径也会顺着执行到标签，因此两个路径都释放内存。Rust 的 `Drop` 经常承担类似的资源清理工作，阅读 C 时需要自己追踪各条路径。

### 宏：先看展开结果

```c
#define BIT(n) (1U << (n))

unsigned int mask = BIT(3);
```

`BIT(3)` 展开为 `(1U << (3))`，值为 `8`，对应将第 3 位设为 1。这与 STM32 寄存器位操作的经验相通。

遇到复杂宏时，先找到定义，理解参数如何展开；`container_of` 等写法留到实际阅读相关代码时再补充。

## 后续路线

1. 完成编译并检查内核、模块产物。
2. 准备实验用户空间，在 QEMU 中启动这个内核。
3. 加载、卸载 `rust_minimal`，查看内核日志和模块参数。
4. 参考 `rust_misc_device.rs` 做虚拟传感器，用普通 Rust 应用读写设备，理解系统调用到驱动回调的路径。
5. 学习并发访问与锁，再迁移到熟悉的 I²C 外设，补设备匹配、设备树与驱动子系统。

## 参考资料

- [Linux 内核 Rust Quick Start](https://docs.kernel.org/rust/quick-start.html)：工具链检查、配置和构建。
- [Linux 内核 Rust General Information](https://docs.kernel.org/rust/general-information.html)：`no_std`、Rust 抽象与 C bindings。
- [Rust for Linux 版本策略](https://rust-for-linux.com/rust-version-policy)：工具链要求以所选内核版本为准。
- [Linux Kernel Labs](https://linux-kernel-labs.github.io/refs/heads/master/)：复习内核模块、字符设备、中断和同步概念；具体 API 要与实验版本核对。
- [Microsoft：WSL 基本命令](https://learn.microsoft.com/en-us/windows/wsl/basic-commands)。
- [Microsoft：WSL 版本比较](https://learn.microsoft.com/en-us/windows/wsl/compare-versions)。
- [QEMU 系统模拟简介](https://www.qemu.org/docs/master/system/introduction.html)。

本仓库保存笔记和精选实验代码。本地的 `rust-linux-lab/` 包含完整源码、压缩包和构建文件，已通过 `.gitignore` 排除。
