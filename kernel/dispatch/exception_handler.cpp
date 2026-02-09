#include "exception_handler.h"
#include <iostream>
#include <stdexcept>

ExceptionManager::ExceptionManager() : exceptionCount(0) {}

void ExceptionManager::handleVmException(uint32_t vmId, const std::string& exceptionType) {
    exceptionCount++;
    std::cout << "Exception occurred in VM " << vmId 
              << ": " << exceptionType << std::endl;
    
    // 记录异常日志
    logException(vmId, exceptionType);
    
    // 根据异常类型采取不同措施
    if (exceptionType == "MEMORY_ACCESS_VIOLATION") {
        handleMemoryViolation(vmId);
    } else if (exceptionType == "RESOURCE_TIMEOUT") {
        handleResourceTimeout(vmId);
    } else if (exceptionType == "INVALID_INSTRUCTION") {
        handleInvalidInstruction(vmId);
    }
}

void ExceptionManager::handleMemoryViolation(uint32_t vmId) {
    std::cout << "Handling memory violation for VM " << vmId << std::endl;
    // 实际项目中会暂停VM并记录详细信息
}

void ExceptionManager::handleResourceTimeout(uint32_t vmId) {
    std::cout << "Handling resource timeout for VM " << vmId << std::endl;
    // 实际项目中会检查资源使用情况并可能终止VM
}

void ExceptionManager::handleInvalidInstruction(uint32_t vmId) {
    std::cout << "Handling invalid instruction for VM " << vmId << std::endl;
    // 实际项目中会记录无效指令并可能暂停VM
}

void ExceptionManager::logException(uint32_t vmId, const std::string& exceptionType) {
    // 简单的日志记录 - 实际项目中会写入日志文件
    std::cout << "[EXCEPTION_LOG] VM:" << vmId 
              << " Type:" << exceptionType 
              << " Count:" << exceptionCount << std::endl;
}

uint32_t ExceptionManager::getExceptionCount() const {
    return exceptionCount;
}

void ExceptionManager::resetExceptionCount() {
    exceptionCount = 0;
    std::cout << "Exception count reset" << std::endl;
}