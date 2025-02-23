#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 平台相关头文件
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <io.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#endif

// 全局运行标志
volatile int keep_running = 1;
volatile int signal_received = 0; // 新增：标记是否接收到信号

// 平台特定的信号/控制台处理
#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        keep_running = 0;
        signal_received = 1; // 设置信号接收标记
        return TRUE;
    }
    return FALSE;
}
#else
void handle_signal(int sig) {
    keep_running = 0;
    signal_received = 1; // 设置信号接收标记
}
#endif

// 跨平台终止进程函数
int kill_process(int pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        fprintf(stderr, "[WinError %lu] OpenProcess failed\n", GetLastError());
        return -1;
    }
    if (!TerminateProcess(hProcess, 1)) {
        fprintf(stderr, "[WinError %lu] TerminateProcess failed\n", GetLastError());
        CloseHandle(hProcess);
        return -1;
    }
    CloseHandle(hProcess);
    return 0;
#else
    if (kill(pid, SIGKILL) == -1) {
        fprintf(stderr, "Error killing PID %d: %s\n", pid, strerror(errno));
        return -1;
    }
    return 0;
#endif
}

int main() {
    // 注册终止信号处理
#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    const char* nvidia_cmd = "nvidia-smi --query-compute-apps=pid --format=csv,noheader";
#else
    signal(SIGINT, handle_signal);
    const char* nvidia_cmd = "nvidia-smi --query-compute-apps=pid --format=csv,noheader";
#endif

    while (keep_running) {
        FILE* cmd = popen(nvidia_cmd, "r");
        if (!cmd) {
#ifdef _WIN32
            fprintf(stderr, "Command failed: %lu\n", GetLastError());
#else
            perror("popen failed");
#endif
            Sleep(1);
            continue;
        }

        char buffer[256];
        int found = 0;

        // 读取并终止所有GPU进程
        while (fgets(buffer, sizeof(buffer), cmd) != NULL) {
            int pid = atoi(buffer);
            if (pid <= 0) continue;

            printf("Killing PID: %d\n", pid);
            if (kill_process(pid) == 0) {
                found = 1;
            }
        }

        pclose(cmd);

        if (!found) {
            printf("No GPU processes found.\n");
        }

#ifdef _WIN32
        Sleep(1000);  // Windows sleep单位毫秒
#else
        sleep(1);     // Linux sleep单位秒
#endif
    }

    printf("\nExiting...\n");

    // 如果由信号触发退出，尝试启动桌面进程
    if (signal_received) {
#ifdef _WIN32
        system("start explorer.exe"); // Windows启动资源管理器
#else
        // Linux尝试启动GNOME或KDE（示例命令，可能需要根据桌面环境调整）
        system("gnome-session --session=ubuntu > /dev/null 2>&1 &"); 
#endif
    }

    return 0;
}