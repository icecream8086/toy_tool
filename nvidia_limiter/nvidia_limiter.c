#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>  // 引入math.h以使用ceil函数

// 条件编译定义平台相关参数
#ifdef _WIN32
    #define NVIDIA_SMI "nvidia-smi.exe"
    #define POPEN _popen
    #define PCLOSE _pclose
#else
    #define NVIDIA_SMI "sudo nvidia-smi"
    #define POPEN popen
    #define PCLOSE pclose
#endif

// 结构体存储显卡信息
typedef struct {
    int default_power;  // 默认功耗（W）
    int min_power;      // 最小允许功耗
    int max_power;      // 最大允许功耗
    
    int core_clock;     // 当前核心频率（MHz）
    int max_core_clock; // 最大核心频率
    
    int mem_clock;      // 当前显存频率（MHz）
    int max_mem_clock;  // 最大显存频率
} GPUInfo;

// 函数声明
void show_help();
int execute_command(const char* cmd, char* buf, size_t buf_size);
int parse_gpu_info(GPUInfo* info);
int set_power_limit(int target_watt);
int set_clocks(int core_mhz, int mem_mhz);
int reset_settings();

int main(int argc, char *argv[]) {
    int rate = -1;
    int reset_flag = 0;

    // 参数解析
    if (argc == 1 || argc > 3) {
        show_help();
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-help") == 0) {
            show_help();
            return 0;
        } else if (strcmp(argv[i], "-reset") == 0) {
            reset_flag = 1;
        } else if (strcmp(argv[i], "-rate") == 0) {
            if (i+1 >= argc || !isdigit(*argv[i+1])) {
                fprintf(stderr, "错误：需要指定-rate参数值\n");
                return 1;
            }
            rate = atoi(argv[++i]);
            if (rate < 50 || rate > 100) {
                fprintf(stderr, "错误：rate参数范围需在50-100之间\n");
                return 1;
            }
        }
    }

    // 执行核心逻辑
    if (reset_flag) {
        return reset_settings();
    } else if (rate != -1) {
        GPUInfo info;
        if (!parse_gpu_info(&info)) {
            fprintf(stderr, "错误：无法获取GPU信息\n");
            return 1;
        }

        // 计算目标值
        int target_power = (int)ceil(info.min_power + (info.max_power - info.min_power) * rate / 100.0);
        int target_core = info.max_core_clock * rate / 100;
        int target_mem = info.max_mem_clock * rate / 100;

        // 确保目标功耗不低于最小允许功耗
        if (target_power < info.min_power) {
            target_power = info.min_power;
        }

        printf("正在设置：\n  功耗:%dW\n  核心频率:%dMHz\n  显存频率:%dMHz\n",
               target_power, target_core, target_mem);

        // 应用设置
        if (set_power_limit(target_power) ||
            set_clocks(target_core, target_mem)) {
            // fprintf(stderr, "错误：设置失败\n");
            return 1;
        }
        return 0;
    }

    show_help();
    return 0;
}

// 执行shell命令并捕获输出
int execute_command(const char* cmd, char* buf, size_t buf_size) {
    FILE* fp = POPEN(cmd, "r");
    if (!fp) return -1;

    size_t len = fread(buf, 1, buf_size-1, fp);
    buf[len] = '\0';
    return PCLOSE(fp);
}

// 解析nvidia-smi输出获取关键参数
int parse_gpu_info(GPUInfo* info) {
    char buf[4096];
    
    // 获取功耗信息
    if (execute_command(NVIDIA_SMI " -q -d POWER", buf, sizeof(buf))) {
        return 0;
    }
    
    // 解析功耗值（示例正则匹配逻辑）
    sscanf(buf, "    Min Power Limit : %d W", &info->min_power);
    sscanf(buf, "    Max Power Limit : %d W", &info->max_power);
    sscanf(buf, "    Power Limit : %d W", &info->default_power);

    // 获取时钟信息
    execute_command(NVIDIA_SMI " -q -d SUPPORTED_CLOCKS", buf, sizeof(buf));
    // 解析最大值（需要实际解析逻辑）
    info->max_core_clock = 1770;  // 根据实际输出解析
    info->max_mem_clock = 1750;

    return 1;
}


// 设置功耗限制
int set_power_limit(int target_watt) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s -pl %d", NVIDIA_SMI, target_watt);
    int result = system(cmd);

    if (result != 0) {
        fprintf(stderr, "警告：当前GPU不支持修改功耗限制\n");
        return 1;
    }
    return 0;
}

// 设置时钟频率
int set_clocks(int core_mhz, int mem_mhz) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s -lgc %d,%d", NVIDIA_SMI, core_mhz, core_mhz);
    if (system(cmd)) return 1;
    
    snprintf(cmd, sizeof(cmd), "%s -lmc %d,%d", NVIDIA_SMI, mem_mhz, mem_mhz);
    return system(cmd);
}

// 恢复默认设置
int reset_settings() {
    GPUInfo info;
    if (!parse_gpu_info(&info)) return 1;

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s -pl %d", NVIDIA_SMI, info.default_power);
    system(cmd);
    system(NVIDIA_SMI " -rgc"); // 重置核心时钟
    system(NVIDIA_SMI " -rmc"); // 重置显存时钟
    return 0;
}

void show_help() {
    printf("使用说明:\n"
           "  nvidia_limiter -rate <50-100>   设置性能百分比\n"
           "  nvidia_limiter -reset          恢复默认设置\n"
           "  nvidia_limiter -help           显示帮助\n\n"
           "示例:\n"
           "  nvidia_limiter -rate 70\n"
           "  nvidia_limiter -reset\n");
}

//todo...