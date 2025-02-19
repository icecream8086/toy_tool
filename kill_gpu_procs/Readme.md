# kill_gpu_procs

## 简介

此 C 程序 **kill_gpu_procs** 旨在帮助用户**终止所有占用 NVIDIA GPU 资源的进程**。它通过 `nvidia-smi` 命令查询 GPU 进程 PID 并终止。程序支持 Windows 和 Linux，可通过 `Ctrl+C` 优雅退出。

## 功能特性

*   **跨平台**: 支持 Windows 和 Linux。
*   **自动清理**: 循环终止 NVIDIA GPU 计算进程。
*   **信号处理**:  `Ctrl+C` 安全退出。
*   **错误处理**:  输出详细错误信息。
*   **简洁**:  编译后即可运行。

## 编译和运行

### 前提条件

*   **NVIDIA 显卡及驱动**
*   **nvidia-smi 工具**
*   **C 编译器** (GCC, MinGW, Visual Studio)

### 编译步骤

#### Linux

1.  打开终端。
2.  编译：

    ```bash
    gcc gpu_killer.c -o kill_gpu_procs
    ```

#### Windows

1.  打开命令提示符/PowerShell (配置好 C 编译器)。
2.  编译 (GCC MinGW)：

    ```bash
    gcc gpu_killer.c -o kill_gpu_procs.exe
    ```

    或 (Visual Studio 开发人员命令提示符)：

    ```bash
    cl gpu_killer.c
    ```

### 运行程序

编译成功后，运行程序。

#### Linux

```bash
./kill_gpu_procs
```

#### Windows

```bash
kill_gpu_procs.exe
```

### 使用方法

1.  **编译**:  根据操作系统编译 `gpu_killer.c`。
2.  **运行**:  在终端/命令提示符运行可执行文件。
3.  **观察**:  程序输出 GPU 进程信息。
4.  **退出**:  `Ctrl+C` 退出。

## 注意事项

*   可能需要**管理员权限**。
*   依赖 **`nvidia-smi` 工具**。
*   程序会**循环运行**，`Ctrl+C` 停止。
*   **谨慎使用，避免误杀**。

## 免责声明

**使用风险自负**。

---