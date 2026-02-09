#include "scheduler.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <sstream>

Scheduler::Scheduler() : isRunning(false), totalCores(0), vmCoreCount(0) {}

Scheduler::~Scheduler() {
    stop();
}

bool Scheduler::initialize() {
    // 获取系统CPU核心数
    totalCores = GetCPUCoreCount();
    if (totalCores <= CORE_START_INDEX) {
        std::cerr << "Error: Insufficient CPU cores for VM scheduling" << std::endl;
        return false;
    }
    
    vmCoreCount = totalCores - CORE_START_INDEX;
    
    // 初始化核心池（从核心2开始）
    corePool.resize(vmCoreCount);
    for (uint32_t i = 0; i < vmCoreCount; i++) {
        corePool[i].coreId = CORE_START_INDEX + i;
        corePool[i].lockStatus = GilLockStatus::UNLOCKED;
        corePool[i].boundVmId = 0;
        corePool[i].isActive = false;
    }
    
    std::cout << "Scheduler initialized: " << vmCoreCount 
              << " cores available for VM scheduling (cores " 
              << CORE_START_INDEX << "-" << (CORE_START_INDEX + vmCoreCount - 1) << ")" << std::endl;
    
    return true;
}

void Scheduler::start() {
    if (isRunning) {
        std::cout << "Scheduler already running" << std::endl;
        return;
    }
    
    isRunning = true;
    schedulerThread = std::thread(&Scheduler::schedulerLoop, this);
    std::cout << "Scheduler started" << std::endl;
}

void Scheduler::stop() {
    if (!isRunning) {
        return;
    }
    
    isRunning = false;
    scheduleCV.notify_all();
    
    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }
    
    // 停止所有VM
    for (auto& binding : staticBindings) {
        if (binding.vmPtr && binding.vmPtr->getRunningStatus()) {
            binding.vmPtr->stop();
        }
    }
    
    while (!dynamicQueue.empty()) {
        auto vmInfo = dynamicQueue.front();
        if (vmInfo.vmPtr && vmInfo.vmPtr->getRunningStatus()) {
            vmInfo.vmPtr->stop();
        }
        dynamicQueue.pop();
    }
    
    std::cout << "Scheduler stopped" << std::endl;
}

bool Scheduler::addVm(std::shared_ptr<X86Vm> vm, uint32_t priority) {
    if (!vm) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    S_VmScheduleInfo vmInfo;
    vmInfo.vmId = vm->getVmId();
    vmInfo.vmPtr = vm;
    vmInfo.priority = priority;
    vmInfo.lastExecutionTime = 0;
    vmInfo.isStaticBound = false;
    vmInfo.boundCoreId = 0;
    
    dynamicQueue.push(vmInfo);
    std::cout << "VM " << vmInfo.vmId << " added to dynamic scheduling queue" << std::endl;
    
    scheduleCV.notify_one();
    return true;
}

bool Scheduler::applyStaticCore(uint32_t vmId, uint32_t coreId) {
    // 检查核心ID合法性
    if (coreId < CORE_START_INDEX || coreId >= totalCores) {
        std::cerr << "Core ID " << coreId << " out of range" << std::endl;
        return false;
    }
    
    uint32_t poolIndex = coreId - CORE_START_INDEX;
    
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    // 检查核心是否已被占用
    if (corePool[poolIndex].lockStatus == GilLockStatus::LOCKED) {
        std::cerr << "Core " << coreId << " already occupied by VM " 
                  << corePool[poolIndex].boundVmId << std::endl;
        return false;
    }
    
    // 查找对应的VM
    S_VmScheduleInfo targetVmInfo;
    bool found = false;
    
    // 先在静态绑定列表中查找
    for (auto& binding : staticBindings) {
        if (binding.vmId == vmId) {
            targetVmInfo = binding;
            found = true;
            break;
        }
    }
    
    // 再在动态队列中查找
    if (!found) {
        std::queue<S_VmScheduleInfo> tempQueue;
        while (!dynamicQueue.empty()) {
            auto vmInfo = dynamicQueue.front();
            dynamicQueue.pop();
            
            if (vmInfo.vmId == vmId) {
                targetVmInfo = vmInfo;  // ✅ 正确：复制值而不是引用
                found = true;
                // 注意：这里不将vmInfo放回tempQueue，相当于从动态队列移除
            } else {
                tempQueue.push(vmInfo);
            }
        }
        dynamicQueue = tempQueue;
    }
    
    if (!found) {
        std::cerr << "VM " << vmId << " not found" << std::endl;
        return false;
    }
    
    // 设置静态绑定
    targetVmInfo.isStaticBound = true;
    targetVmInfo.boundCoreId = coreId;
    staticBindings.push_back(targetVmInfo);  // ✅ 正确：使用有效的副本
    
    // 锁定核心
    corePool[poolIndex].lockStatus = GilLockStatus::LOCKED;
    corePool[poolIndex].boundVmId = vmId;
    
    std::cout << "VM " << vmId << " statically bound to core " << coreId << std::endl;
    return true;
}

bool Scheduler::releaseStaticCore(uint32_t vmId) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    auto it = std::find_if(staticBindings.begin(), staticBindings.end(),
                          [vmId](const S_VmScheduleInfo& info) {
                              return info.vmId == vmId;
                          });
    
    if (it == staticBindings.end()) {
        std::cerr << "VM " << vmId << " not found in static bindings" << std::endl;
        return false;
    }
    
    uint32_t coreId = it->boundCoreId;
    uint32_t poolIndex = coreId - CORE_START_INDEX;
    
    // 释放核心锁
    corePool[poolIndex].lockStatus = GilLockStatus::UNLOCKED;
    corePool[poolIndex].boundVmId = 0;
    
    // 停止VM
    if (it->vmPtr && it->vmPtr->getRunningStatus()) {
        it->vmPtr->stop();
    }
    
    // 移除静态绑定
    staticBindings.erase(it);
    
    std::cout << "VM " << vmId << " released from core " << coreId << std::endl;
    return true;
}

S_CoreStatus Scheduler::getCoreStatus(uint32_t coreId) const {
    if (coreId < CORE_START_INDEX || coreId >= totalCores) {
        return S_CoreStatus(); // 返回默认状态
    }
    
    uint32_t poolIndex = coreId - CORE_START_INDEX;
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(schedulerMutex));
    return corePool[poolIndex];
}

std::string Scheduler::getStatistics() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(schedulerMutex));
    
    std::ostringstream oss;
    oss << "=== Scheduler Statistics ===" << std::endl;
    oss << "Total Cores: " << totalCores << std::endl;
    oss << "VM Cores Available: " << vmCoreCount << std::endl;
    oss << "Static Bindings: " << staticBindings.size() << std::endl;
    oss << "Dynamic Queue Size: " << dynamicQueue.size() << std::endl;
    oss << "Core Status:" << std::endl;
    
    for (uint32_t i = 0; i < vmCoreCount; i++) {
        const auto& core = corePool[i];
        oss << "  Core " << core.coreId << ": ";
        oss << (core.lockStatus == GilLockStatus::LOCKED ? "LOCKED" : "FREE");
        if (core.lockStatus == GilLockStatus::LOCKED) {
            oss << " (VM " << core.boundVmId << ")";
        }
        oss << std::endl;
    }
    
    return oss.str();
}

void Scheduler::schedulerLoop() {
    while (isRunning) {
        {
            std::unique_lock<std::mutex> lock(schedulerMutex);
            scheduleCV.wait_for(lock, std::chrono::milliseconds(TIME_SLICE_MS),
                               [this] { return !isRunning; });
        }
        
        if (!isRunning) break;
        
        executeStaticBindings();
        executeDynamicScheduling();
        checkTimeoutVms();
    }
}

void Scheduler::executeDynamicScheduling() {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    if (dynamicQueue.empty()) {
        return;
    }
    
    // 按优先级排序（简单实现：优先级数字小的优先）
    std::vector<S_VmScheduleInfo> sortedVms;
    std::queue<S_VmScheduleInfo> tempQueue = dynamicQueue;
    
    while (!tempQueue.empty()) {
        sortedVms.push_back(tempQueue.front());
        tempQueue.pop();
    }
    
    std::sort(sortedVms.begin(), sortedVms.end(),
              [](const S_VmScheduleInfo& a, const S_VmScheduleInfo& b) {
                  return a.priority < b.priority;
              });
    
    // 执行动态调度
    for (auto& vmInfo : sortedVms) {
        if (!isRunning) break;
        
        int coreId = getAvailableCore();
        if (coreId == -1) {
            // 无可用核心，放回队列末尾
            dynamicQueue.push(vmInfo);
            continue;
        }
        
        uint32_t poolIndex = coreId - CORE_START_INDEX;
        
        // 绑定核心
        corePool[poolIndex].lockStatus = GilLockStatus::LOCKED;
        corePool[poolIndex].boundVmId = vmInfo.vmId;
        
        // 设置线程亲和性
        if (SetThreadCPUAffinity(coreId) != 0) {
            std::cerr << "Warning: Failed to set thread affinity to core " << coreId << std::endl;
        }
        
        // 执行VM时间片
        if (vmInfo.vmPtr) {
            if (!vmInfo.vmPtr->getRunningStatus()) {
                vmInfo.vmPtr->start();
            }
            vmInfo.vmPtr->runOneSlice();
            vmInfo.lastExecutionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }
        
        // 释放核心
        releaseCoreLock(coreId);
        
        // 放回队列末尾
        dynamicQueue.push(vmInfo);
    }
}

void Scheduler::executeStaticBindings() {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    for (auto& binding : staticBindings) {
        if (!isRunning) break;
        
        uint32_t coreId = binding.boundCoreId;
        uint32_t poolIndex = coreId - CORE_START_INDEX;
        
        // 确保核心仍被锁定
        if (corePool[poolIndex].lockStatus != GilLockStatus::LOCKED ||
            corePool[poolIndex].boundVmId != binding.vmId) {
            continue;
        }
        
        // 设置线程亲和性
        if (SetThreadCPUAffinity(coreId) != 0) {
            std::cerr << "Warning: Failed to set thread affinity to core " << coreId << std::endl;
        }
        
        // 执行VM时间片
        if (binding.vmPtr) {
            if (!binding.vmPtr->getRunningStatus()) {
                binding.vmPtr->start();
            }
            binding.vmPtr->runOneSlice();
            binding.lastExecutionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }
    }
}

int Scheduler::getAvailableCore() {
    for (uint32_t i = 0; i < vmCoreCount; i++) {
        if (corePool[i].lockStatus == GilLockStatus::UNLOCKED) {
            return corePool[i].coreId;
        }
    }
    return -1; // 无可用核心
}

void Scheduler::releaseCoreLock(uint32_t coreId) {
    if (coreId < CORE_START_INDEX || coreId >= totalCores) {
        return;
    }
    
    uint32_t poolIndex = coreId - CORE_START_INDEX;
    corePool[poolIndex].lockStatus = GilLockStatus::UNLOCKED;
    corePool[poolIndex].boundVmId = 0;
}

void Scheduler::checkTimeoutVms() {
    // 简单的超时检查实现
    // 实际项目中可以根据执行时间和资源使用情况进行更复杂的超时处理
    const uint64_t TIMEOUT_THRESHOLD = 5000; // 5秒超时阈值
    
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    // 检查静态绑定VM
    for (auto& binding : staticBindings) {
        if (currentTime - binding.lastExecutionTime > TIMEOUT_THRESHOLD) {
            std::cout << "Warning: VM " << binding.vmId << " may be timeout" << std::endl;
            // 实际项目中可以采取暂停、重启或其他措施
        }
    }
}