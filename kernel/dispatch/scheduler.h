#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <cstdint>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include "../Cross_PlatformUnifiedMacro.h"
#include "../CPUvm/x86Vm.h"

/**
 * @brief GIL锁状态枚举
 */
enum class GilLockStatus {
    UNLOCKED = 0,    // 未锁定
    LOCKED = 1       // 已锁定
};

/**
 * @brief 核心状态结构体
 */
struct S_CoreStatus {
    uint32_t coreId;                    // 核心ID
    GilLockStatus lockStatus;           // 锁状态
    uint32_t boundVmId;                 // 绑定的VM ID（0表示未绑定）
    bool isActive;                      // 是否活跃（使用普通bool替代atomic）
    
    S_CoreStatus() : coreId(0), lockStatus(GilLockStatus::UNLOCKED), 
                     boundVmId(0), isActive(false) {}
    
    // 显式定义拷贝构造函数
    S_CoreStatus(const S_CoreStatus& other) 
        : coreId(other.coreId), lockStatus(other.lockStatus),
          boundVmId(other.boundVmId), isActive(other.isActive) {}
    
    // 显式定义赋值操作符
    S_CoreStatus& operator=(const S_CoreStatus& other) {
        if (this != &other) {
            coreId = other.coreId;
            lockStatus = other.lockStatus;
            boundVmId = other.boundVmId;
            isActive = other.isActive;
        }
        return *this;
    }
};

/**
 * @brief VM调度信息结构体
 */
struct S_VmScheduleInfo {
    uint32_t vmId;                      // VM ID
    std::shared_ptr<X86Vm> vmPtr;       // VM智能指针
    uint32_t priority;                  // 优先级（数值越小优先级越高）
    uint64_t lastExecutionTime;         // 上次执行时间戳
    bool isStaticBound;                 // 是否静态绑定核心
    uint32_t boundCoreId;               // 绑定的核心ID
    
    S_VmScheduleInfo() : vmId(0), priority(10), lastExecutionTime(0), 
                        isStaticBound(false), boundCoreId(0) {}
};

/**
 * @brief 调度器类，负责VM的调度和核心管理
 * @details 实现GIL式核锁保护和时间片调度机制
 */
class Scheduler {
private:
    static const uint32_t TIME_SLICE_MS = 10;       // 时间片大小（毫秒）
    static const uint32_t CORE_START_INDEX = 2;     // VM可用核心起始索引
    
    std::vector<S_CoreStatus> corePool;             // 核心池
    std::queue<S_VmScheduleInfo> dynamicQueue;      // 动态调度队列
    std::vector<S_VmScheduleInfo> staticBindings;   // 静态绑定列表
    std::mutex schedulerMutex;                      // 调度器互斥锁
    std::condition_variable scheduleCV;             // 调度条件变量
    std::atomic<bool> isRunning;                    // 调度器运行状态
    std::thread schedulerThread;                    // 调度器线程
    uint32_t totalCores;                            // 总核心数
    uint32_t vmCoreCount;                           // VM可用核心数
    
public:
    Scheduler();
    ~Scheduler();
    
    /**
     * @brief 初始化调度器
     * @return bool 初始化成功返回true
     */
    bool initialize();
    
    /**
     * @brief 启动调度器
     */
    void start();
    
    /**
     * @brief 停止调度器
     */
    void stop();
    
    /**
     * @brief 添加VM到调度器
     * @param vm VM智能指针
     * @param priority 优先级
     * @return bool 添加成功返回true
     */
    bool addVm(std::shared_ptr<X86Vm> vm, uint32_t priority = 10);
    
    /**
     * @brief 申请静态核心绑定
     * @param vmId VM ID
     * @param coreId 请求的核心ID
     * @return bool 申请成功返回true
     */
    bool applyStaticCore(uint32_t vmId, uint32_t coreId);
    
    /**
     * @brief 释放静态核心绑定
     * @param vmId VM ID
     * @return bool 释放成功返回true
     */
    bool releaseStaticCore(uint32_t vmId);
    
    /**
     * @brief 获取核心状态
     * @param coreId 核心ID
     * @return S_CoreStatus 核心状态
     */
    S_CoreStatus getCoreStatus(uint32_t coreId) const;
    
    /**
     * @brief 获取调度器统计信息
     * @return std::string 统计信息字符串
     */
    std::string getStatistics() const;
    
private:
    /**
     * @brief 调度器主循环
     */
    void schedulerLoop();
    
    /**
     * @brief 执行动态调度
     */
    void executeDynamicScheduling();
    
    /**
     * @brief 执行静态绑定VM
     */
    void executeStaticBindings();
    
    /**
     * @brief 获取可用核心
     * @return int 可用核心ID，-1表示无可用核心
     */
    int getAvailableCore();
    
    /**
     * @brief 释放核心锁
     * @param coreId 核心ID
     */
    void releaseCoreLock(uint32_t coreId);
    
    /**
     * @brief 检查并处理超时VM
     */
    void checkTimeoutVms();
};

#endif // SCHEDULER_H