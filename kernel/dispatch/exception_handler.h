#ifndef EXCEPTION_HANDLER_H
#define EXCEPTION_HANDLER_H

#include <cstdint>
#include <string>

/**
 * @brief 异常管理器类，处理VM运行时的各种异常情况
 * @details 包括内存越界、资源超时、无效指令等异常的检测和处理
 */
class ExceptionManager {
private:
    uint32_t exceptionCount;    // 异常计数器
    
public:
    ExceptionManager();
    
    /**
     * @brief 处理VM异常
     * @param vmId 发生异常的VM标识符
     * @param exceptionType 异常类型
     */
    void handleVmException(uint32_t vmId, const std::string& exceptionType);
    
    /**
     * @brief 处理内存越界异常
     * @param vmId VM标识符
     */
    void handleMemoryViolation(uint32_t vmId);
    
    /**
     * @brief 处理资源超时异常
     * @param vmId VM标识符
     */
    void handleResourceTimeout(uint32_t vmId);
    
    /**
     * @brief 处理无效指令异常
     * @param vmId VM标识符
     */
    void handleInvalidInstruction(uint32_t vmId);
    
    /**
     * @brief 记录异常日志
     * @param vmId VM标识符
     * @param exceptionType 异常类型
     */
    void logException(uint32_t vmId, const std::string& exceptionType);
    
    // Getter方法
    uint32_t getExceptionCount() const;
    void resetExceptionCount();
};

#endif // EXCEPTION_HANDLER_H