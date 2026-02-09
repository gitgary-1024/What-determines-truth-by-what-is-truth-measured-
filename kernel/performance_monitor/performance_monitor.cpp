#include "performance_monitor.h"
#include <iostream>
#include <chrono>

PerformanceMonitor::PerformanceMonitor() 
    : startTime(std::chrono::high_resolution_clock::now()),
      totalInstructions(0),
      activeVms(0) {}

void PerformanceMonitor::recordVmStart(uint32_t vmId) {
    auto now = std::chrono::high_resolution_clock::now();
    vmStartTimes[vmId] = now;
    activeVms++;
    std::cout << "Performance monitor: VM " << vmId << " started" << std::endl;
}

void PerformanceMonitor::recordVmStop(uint32_t vmId, uint32_t instructionCount) {
    auto now = std::chrono::high_resolution_clock::now();
    auto it = vmStartTimes.find(vmId);
    
    if (it != vmStartTimes.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        vmExecutionTimes[vmId] = duration.count();
        totalInstructions += instructionCount;
        activeVms--;
        
        std::cout << "Performance monitor: VM " << vmId 
                  << " stopped after " << instructionCount << " instructions in "
                  << duration.count() << " ms" << std::endl;
    }
}

double PerformanceMonitor::getAverageExecutionTime() const {
    if (vmExecutionTimes.empty()) {
        return 0.0;
    }
    
    uint64_t totalTime = 0;
    for (const auto& pair : vmExecutionTimes) {
        totalTime += pair.second;
    }
    
    return static_cast<double>(totalTime) / vmExecutionTimes.size();
}

double PerformanceMonitor::getInstructionsPerSecond() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    
    if (totalTime.count() == 0) {
        return 0.0;
    }
    
    return static_cast<double>(totalInstructions) / totalTime.count();
}

uint32_t PerformanceMonitor::getActiveVmCount() const {
    return activeVms;
}

uint32_t PerformanceMonitor::getTotalInstructions() const {
    return totalInstructions;
}

void PerformanceMonitor::printPerformanceReport() const {
    std::cout << "\n=== Performance Report ===" << std::endl;
    std::cout << "Active VMs: " << activeVms << std::endl;
    std::cout << "Total Instructions Executed: " << totalInstructions << std::endl;
    std::cout << "Average Execution Time: " << getAverageExecutionTime() << " ms" << std::endl;
    std::cout << "Instructions Per Second: " << getInstructionsPerSecond() << std::endl;
    
    if (!vmExecutionTimes.empty()) {
        std::cout << "\nIndividual VM Performance:" << std::endl;
        for (const auto& pair : vmExecutionTimes) {
            std::cout << "  VM " << pair.first << ": " << pair.second << " ms" << std::endl;
        }
    }
    std::cout << "========================\n" << std::endl;
}