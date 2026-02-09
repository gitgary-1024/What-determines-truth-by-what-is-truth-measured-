#ifndef Cross_PlatformUnifiedMacro_H_
#define Cross_PlatformUnifiedMacro_H_

/************************* 基础平台识别 *************************/
// Windows 平台（32/64位）
#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
    // 区分32/64位Windows
    #ifdef _WIN64
        #define PLATFORM_WINDOWS_64 1
        #define PLATFORM_64BIT 1
    #else
        #define PLATFORM_WINDOWS_32 1
        #define PLATFORM_32BIT 1
    #endif
// Linux 平台
#elif defined(__linux__) || defined(__linux)
    #define PLATFORM_LINUX 1
    #define PLATFORM_UNIX_LIKE 1
    // 区分32/64位Linux
    #if __SIZEOF_POINTER__ == 8
        #define PLATFORM_64BIT 1
    #else
        #define PLATFORM_32BIT 1
    #endif
// macOS 平台
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS 1
    #define PLATFORM_UNIX_LIKE 1
    #if __SIZEOF_POINTER__ == 8
        #define PLATFORM_64BIT 1
    #else
        #define PLATFORM_32BIT 1
    #endif
// 未知平台
#else
    #define PLATFORM_UNKNOWN 1
#endif

/************************* 编译器识别 *************************/
#ifdef _MSC_VER
    #define COMPILER_MSVC 1          // Visual Studio 编译器
    #define COMPILER_MSVC_VER _MSC_VER // MSVC 版本号（如1920对应VS2019）
#elif defined(__GNUC__)
    #define COMPILER_GCC 1           // GCC 编译器
    #define COMPILER_GCC_VER __GNUC__ // GCC 主版本号
#elif defined(__clang__)
    #define COMPILER_CLANG 1         // Clang 编译器
#endif

/************************* 基础系统头文件引入 *************************/
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <direct.h>    // Windows 路径操作
    #include <io.h>        // Windows IO 操作
#else
    #include <unistd.h>    // Unix/Linux/macOS 基础接口
    #include <sys/stat.h>  // Unix/Linux/macOS 文件状态
    #include <sys/types.h> // Unix/Linux/macOS 类型定义
    #include <dirent.h>    // Unix/Linux/macOS 目录操作
    #include <pthread.h>   // Linux/macOS 线程操作（CPU亲和性需要）
    #include <sys/sysinfo.h> // Linux 获取CPU数补充
#endif

/************************* 通用字符/路径宏 *************************/
// 路径分隔符（Windows：\，Unix/Linux/macOS：/）
#ifdef PLATFORM_WINDOWS
    #define PATH_SEPARATOR "\\"
    #define PATH_SEPARATOR_CHAR '\\'
#else
    #define PATH_SEPARATOR "/"
    #define PATH_SEPARATOR_CHAR '/'
#endif

// 换行符（Windows：\r\n，Unix/Linux/macOS：\n）
#ifdef PLATFORM_WINDOWS
    #define NEWLINE "\r\n"
#else
    #define NEWLINE "\n"
#endif

// 环境变量分隔符（Windows：;，Unix/Linux/macOS：:）
#ifdef PLATFORM_WINDOWS
    #define ENV_SEPARATOR ";"
#else
    #define ENV_SEPARATOR ":"
#endif

/************************* 库导出/导入宏 *************************/
// 用于动态库导出（DLL/SO/DYLIB）
#ifdef PLATFORM_WINDOWS
    // Windows 下：导出时用__declspec(dllexport)，导入时用__declspec(dllimport)
    #ifdef LIB_EXPORT // 定义LIB_EXPORT时表示当前编译动态库（导出符号）
        #define API_EXPORT __declspec(dllexport)
    #else
        #define API_EXPORT __declspec(dllimport)
    #endif
#else
    // Unix/Linux/macOS 下：使用GCC/Clang的可见性属性
    #define API_EXPORT __attribute__((visibility("default")))
#endif

/************************* 通用函数封装 *************************/
// 毫秒级休眠（跨平台统一接口）
#ifdef PLATFORM_WINDOWS
    // Windows Sleep 接收毫秒数
    #define SleepMs(ms) Sleep((DWORD)(ms))
#else
    // Unix/Linux/macOS usleep 接收微秒数（1ms=1000us）
    #define SleepMs(ms) usleep((useconds_t)((ms) * 1000))
#endif

// 获取当前工作目录（跨平台统一接口）
#ifdef PLATFORM_WINDOWS
    #define GetCurrentDir(buf, size) _getcwd((buf), (size))
#else
    #define GetCurrentDir(buf, size) getcwd((buf), (size))
#endif

// 文件是否存在（跨平台统一接口）
static inline int FileExists(const char* path) {
#ifdef PLATFORM_WINDOWS
    return (_access(path, 0) == 0) ? 1 : 0;
#else
    return (access(path, F_OK) == 0) ? 1 : 0;
#endif
}

// 获取系统CPU核心数（跨平台统一接口）
// 返回值：CPU核心数（失败返回-1）
static inline int GetCPUCoreCount() {
#ifdef PLATFORM_WINDOWS
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return (int)sysInfo.dwNumberOfProcessors;
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
    long coreCount = sysconf(_SC_NPROCESSORS_ONLN); // 获取在线可用的CPU核心数
    return (coreCount > 0) ? (int)coreCount : -1;
#else
    return -1; // 未知平台
#endif
}

// 设置当前线程绑定到指定CPU核心（跨平台统一接口）
// 参数：cpu_index - CPU核心索引（从0开始，如0表示第1个核心）
// 返回值：0=成功，非0=失败
static inline int SetThreadCPUAffinity(int cpu_index) {
    // 先校验CPU索引合法性
    int coreCount = GetCPUCoreCount();
    if (cpu_index < 0 || cpu_index >= coreCount) {
        return -1; // 索引超出范围
    }

#ifdef PLATFORM_WINDOWS
    // Windows：SetThreadAffinityMask 接收CPU掩码（1<<cpu_index表示绑定到第cpu_index个核心）
    // GetCurrentThread() 返回当前线程句柄
    DWORD_PTR mask = (DWORD_PTR)1 << cpu_index;
    DWORD_PTR result = SetThreadAffinityMask(GetCurrentThread(), mask);
    return (result == 0) ? -2 : 0; // result为0表示失败
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
    // Linux/macOS：使用pthread_setaffinity_np 设置CPU亲和性
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);          // 清空CPU集合
    CPU_SET(cpu_index, &cpuset); // 将指定CPU加入集合
    // 绑定当前线程到指定CPU集合
    int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    return ret; // 成功返回0，失败返回错误码
#else
    return -3; // 未知平台不支持
#endif
}

/************************* 兼容性宏（避免重复定义） *************************/
// 修复Windows下缺少的Unix风格宏
#ifdef PLATFORM_WINDOWS
    #define S_IRUSR _S_IREAD  // 读权限（用户）
    #define S_IWUSR _S_IWRITE // 写权限（用户）
    #define S_IXUSR _S_IEXEC  // 执行权限（用户）
    // 定义ssize_t（Windows默认无此类型）
    #ifndef ssize_t
        typedef intptr_t ssize_t;
    #endif
#endif

#endif // Cross_PlatformUnifiedMacro_H_