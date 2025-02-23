/*
* wifi_optimizer.c
* 编译命令：
* Linux: gcc wifi_optimizer.c -o wifi_optimizer -Wno-deprecated-declarations && sudo ./wifi_optimizer
* Windows: cl.exe wifi_optimizer.c && .\wifi_optimizer.exe
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#define MAX_CONFIG 10
#define MAX_LINE 256
#define MAX_RETRY 3
#define CONNECT_TIMEOUT 5 // seconds

typedef struct {
    char ssid[64];
    char password[64];
    char domain[64];
} WifiConfig;

typedef struct {
    int (*connect)(const char* ssid, const char* password);
    int (*disconnect)(void);
    float (*ping)(const char* domain);
} PlatformOps;

// 全局平台操作实例
static PlatformOps ops;
static volatile sig_atomic_t keep_running = 1;

void cleanup(int sig) {
    printf("\n正在断开连接并退出...\n");
    ops.disconnect();
    keep_running = 0;
    exit(0);
}

void check_privileges() {
#ifdef _WIN32
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            isAdmin = Elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }

    if (!isAdmin) {
        fprintf(stderr, "需要管理员权限运行！\n");
        exit(1);
    }
#else
    if (geteuid() != 0) {
        fprintf(stderr, "需要root权限运行！\n");
        exit(1);
    }
#endif
}

int parse_config(const char* filename, WifiConfig* configs, int* count) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("无法打开配置文件");
        return -1;
    }

    char line[MAX_LINE];
    *count = 0;
    
    while (fgets(line, sizeof(line), fp) && keep_running) {
        char* cleaned = line;
        while (isspace(*cleaned)) cleaned++;
        if (*cleaned == '#' || *cleaned == '\0') continue;

        char* saveptr;
        char* ssid = strtok_r(cleaned, ",\n", &saveptr);
        char* pass = strtok_r(NULL, ",\n", &saveptr);
        char* domain = strtok_r(NULL, ",\n", &saveptr);

        if (ssid && pass && domain) {
            strncpy(configs[*count].ssid, ssid, sizeof(configs[*count].ssid));
            strncpy(configs[*count].password, pass, sizeof(configs[*count].password));
            strncpy(configs[*count].domain, domain, sizeof(configs[*count].domain));
            (*count)++;
            
            if (*count >= MAX_CONFIG) break;
        }
    }
    fclose(fp);
    return (*count > 0) ? 0 : -1;
}

#ifdef _WIN32
int win_connect(const char* ssid, const char* password) {
    char cmd[512];
    // 生成临时XML配置文件
    const char* profile_fmt = 
        "<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">\n"
        "    <name>%s</name>\n"
        "    <SSIDConfig><SSID><name>%s</name></SSID></SSIDConfig>\n"
        "    <security>\n"
        "        <auth>WPA2</auth>\n"
        "        <encryption>AES</encryption>\n"
        "        <sharedKey>\n"
        "            <keyMaterial>%s</keyMaterial>\n"
        "        </sharedKey>\n"
        "    </security>\n"
        "</WLANProfile>";
    
    FILE* fp = fopen("temp.xml", "w");
    if (!fp) return -1;
    fprintf(fp, profile_fmt, ssid, ssid, password);
    fclose(fp);

    snprintf(cmd, sizeof(cmd), 
        "netsh wlan add profile filename=\"temp.xml\" && "
        "netsh wlan connect name=\"%s\"", ssid);
    return system(cmd);
}

float win_ping(const char* domain) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "ping -n 2 %s", domain);
    
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return -1;

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) && keep_running) {
        if (strstr(buffer, "Average")) {
            char* p = strchr(buffer, '=');
            _pclose(pipe);
            return p ? atof(p + 2) : -1;
        }
    }
    _pclose(pipe);
    return -1.0f;
}

int win_disconnect() {
    return system("netsh wlan disconnect");
}

#else
int lin_connect(const char* ssid, const char* password) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "nmcli --wait %d device wifi connect '%s' password '%s'", 
        CONNECT_TIMEOUT, ssid, password);
    return system(cmd);
}

float lin_ping(const char* domain) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "ping -c 2 -W 1 %s", domain);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return -1;

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) && keep_running) {
        if (strstr(buffer, "avg")) {
            char* p = strchr(buffer, '/');
            pclose(pipe);
            return p ? atof(p + 1) : -1;
        }
    }
    pclose(pipe);
    return -1.0f;
}

int lin_disconnect() {
    return system("nmcli dev disconnect");
}
#endif

int main() {
    signal(SIGINT, cleanup);
    check_privileges();

    WifiConfig configs[MAX_CONFIG];
    int count = 0;
    
    if (parse_config("lan.conf", configs, &count) != 0) {
        fprintf(stderr, "配置文件错误或无有效配置\n");
        return 1;
    }

#ifdef _WIN32
    ops.connect = win_connect;
    ops.disconnect = win_disconnect;
    ops.ping = win_ping;
#else
    ops.connect = lin_connect;
    ops.disconnect = lin_disconnect;
    ops.ping = lin_ping;
#endif

    int best_index = -1;
    float min_latency = 9999.0f;

    for (int i = 0; i < count && keep_running; ++i) {
        printf("测试网络: %-15s => ", configs[i].ssid);
        
        int retries = MAX_RETRY;
        int connected = 0;
        
        while (retries-- > 0 && keep_running) {
            if (ops.connect(configs[i].ssid, configs[i].password) == 0) {
                connected = 1;
                break;
            }
#ifdef _WIN32
            Sleep(2000);
#else
            sleep(2);
#endif
        }

        if (!connected) {
            printf("连接失败\n");
            continue;
        }

#ifdef _WIN32
        Sleep(3000);
#else
        sleep(3);
#endif

        float latency = ops.ping(configs[i].domain);
        if (latency < 0) {
            printf("Ping失败\n");
            latency = 9999.0f;
        } else {
            printf("延迟: %.2f ms\n", latency);
        }

        if (latency < min_latency) {
            min_latency = latency;
            best_index = i;
        }

        ops.disconnect();
    }

    if (best_index != -1 && keep_running) {
        printf("\n▶ 最佳网络: %s (延迟: %.2f ms)\n", 
            configs[best_index].ssid, min_latency);
        ops.connect(configs[best_index].ssid, 
                   configs[best_index].password);
    } else {
        printf("无可用网络\n");
    }

    return 0;
}
