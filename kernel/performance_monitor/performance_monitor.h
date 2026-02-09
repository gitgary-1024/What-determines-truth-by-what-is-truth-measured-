#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <cstdint>
#include <unordered_map>
#include <chrono>

/**
 * @brief 性能监控器类，监控VM系统的运行性能
 * @details 记录VM执行时间、指令数量、活跃VM数量等性能指标
 */
class PerformanceMonitor {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    std::unordered_map<uint32_t, std::chrono::time_point<std::chrono::high_resolution_clock>> vmStartTimes;
    std::unordered_map<uint32_t, uint64_t> vmExecutionTimes;
    uint32_t totalInstructions;
    uint32_t activeVms;
    
public:
    PerformanceMonitor();
    
    /**
     * @brief 记录VM启动时间
     * @param vmId VM标识符
     */
    void recordVmStart(uint32_t vmId);
    
    /**
     * @brief 记录VM停止时间和指令执行数量
     * @param vmId VM标识符
     * @param instructionCount 执行的指令数量
     */
    void recordVmStop(uint32_t vmId, uint32_t instructionCount);
    
    /**
     * @brief 获取平均执行时间
     * @return 平均执行时间（毫秒）
     */
    double getAverageExecutionTime() const;
    
    /**
     * @brief 获取每秒执行指令数
     * @return 每秒指令数
     */
    double getInstructionsPerSecond() const;
    
    /**
     * @brief 获取活跃VM数量
     * @return 当前活跃的VM数量
     */
    uint32_t getActiveVmCount() const;
    
    /**
     * @brief 获取总执行指令数
     * @return 总指令数
     */
    uint32_t getTotalInstructions() const;
    
    /**
     * @brief 打印性能报告
     */
    void printPerformanceReport() const;
};

#endif // PERFORMANCE_MONITOR_H