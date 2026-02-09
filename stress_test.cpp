#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <random>
#include "kernel/CPUvm/x86Vm.h"
#include "kernel/CPUvm/armVm.h"
#include "kernel/CPUvm/x64Vm.h"
#include "kernel/performance_monitor/performance_monitor.h"

/**
 * @brief 压力测试类
 */
class StressTester {
private:
    std::unique_ptr<PerformanceMonitor> perfMonitor;
    std::vector<uint8_t> testPayload;
    
public:
    StressTester() {
        perfMonitor.reset(new PerformanceMonitor());
        // 创建测试payload
        testPayload = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    }
    
    /**
     * @brief 运行并发VM测试
     */
    void runConcurrentVmTest(int numVms, int instructionsPerVm) {
        std::cout << "\n=== Concurrent VM Stress Test ===" << std::endl;
        std::cout << "Creating " << numVms << " VMs with " << instructionsPerVm << " instructions each" << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // 创建VM向量
        std::vector<std::shared_ptr<I_VmInterface>> vms;
        
        // 创建不同类型VM的混合
        for (int i = 0; i < numVms; i++) {
            std::shared_ptr<I_VmInterface> vm;
            switch (i % 3) {
                case 0:
                    vm = std::make_shared<X86Vm>(i + 1);
                    break;
                case 1:
                    vm = std::make_shared<ArmVm>(i + 1);
                    break;
                case 2:
                    vm = std::make_shared<X64Vm>(i + 1);
                    break;
            }
            
            vm->setPayload(testPayload.data(), testPayload.size());
            vms.push_back(vm);
        }
        
        std::cout << "VMs created, starting concurrent execution..." << std::endl;
        
        // 启动所有VM的性能监控
        for (int i = 0; i < numVms; i++) {
            perfMonitor->recordVmStart(vms[i]->getVmId());
        }
        
        // 并发执行所有VM
        std::vector<std::thread> threads;
        std::vector<int> instructionCounts(numVms, 0);
        
        for (int i = 0; i < numVms; i++) {
            threads.emplace_back([this, &vms, i, instructionsPerVm, &instructionCounts]() {
                auto& vm = vms[i];
                int executed = 0;
                
                try {
                    vm->start();
                    
                    for (int j = 0; j < instructionsPerVm; j++) {
                        if (vm->runOneInstruction()) {
                            executed++;
                        }
                    }
                    
                    vm->stop();
                } catch (const std::exception& e) {
                    std::cerr << "VM " << vm->getVmId() << " error: " << e.what() << std::endl;
                }
                
                instructionCounts[i] = executed;
            });
        }
        
        // 等待所有线程完成
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        // 停止性能监控
        for (int i = 0; i < numVms; i++) {
            perfMonitor->recordVmStop(vms[i]->getVmId(), instructionCounts[i]);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 输出结果
        std::cout << "\n=== Stress Test Results ===" << std::endl;
        std::cout << "Total VMs: " << numVms << std::endl;
        std::cout << "Instructions per VM: " << instructionsPerVm << std::endl;
        std::cout << "Total execution time: " << duration.count() << " ms" << std::endl;
        std::cout << "Average time per VM: " << (duration.count() / static_cast<double>(numVms)) << " ms" << std::endl;
        
        // 计算总指令数
        int totalInstructions = 0;
        for (int count : instructionCounts) {
            totalInstructions += count;
        }
        
        std::cout << "Total instructions executed: " << totalInstructions << std::endl;
        std::cout << "Instructions per second: " << (totalInstructions / (duration.count() / 1000.0)) << std::endl;
        
        // 显示性能报告
        perfMonitor->printPerformanceReport();
    }
    
    /**
     * @brief 运行长时间运行测试
     */
    void runLongRunningTest(int durationSeconds) {
        std::cout << "\n=== Long Running Test ===" << std::endl;
        std::cout << "Running VMs for " << durationSeconds << " seconds..." << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        auto endTime = startTime + std::chrono::seconds(durationSeconds);
        
        // 创建几个VM
        std::vector<std::shared_ptr<I_VmInterface>> vms = {
            std::make_shared<X86Vm>(1),
            std::make_shared<ArmVm>(2),
            std::make_shared<X64Vm>(3)
        };
        
        // 设置payload
        for (auto& vm : vms) {
            vm->setPayload(testPayload.data(), testPayload.size());
        }
        
        // 启动性能监控
        for (auto& vm : vms) {
            perfMonitor->recordVmStart(vm->getVmId());
        }
        
        // 运行直到时间结束
        std::vector<int> instructionCounts(vms.size(), 0);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 5000); // 随机间隔
        
        while (std::chrono::high_resolution_clock::now() < endTime) {
            for (size_t i = 0; i < vms.size(); i++) {
                auto& vm = vms[i];
                try {
                    if (!vm->getRunningStatus()) {
                        vm->start();
                    }
                    
                    // 执行随机数量的指令
                    int instructionsToRun = dis(gen);
                    for (int j = 0; j < instructionsToRun; j++) {
                        if (vm->runOneInstruction()) {
                            instructionCounts[i]++;
                        } else {
                            break;
                        }
                    }
                    
                } catch (const std::exception& e) {
                    std::cerr << "VM " << vm->getVmId() << " error: " << e.what() << std::endl;
                }
            }
            
            // 短暂休息
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // 停止所有VM
        for (auto& vm : vms) {
            if (vm->getRunningStatus()) {
                vm->stop();
            }
        }
        
        // 停止性能监控
        for (size_t i = 0; i < vms.size(); i++) {
            perfMonitor->recordVmStop(vms[i]->getVmId(), instructionCounts[i]);
        }
        
        std::cout << "Long running test completed!" << std::endl;
        perfMonitor->printPerformanceReport();
    }
};

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "    MyOS VM System Stress Testing" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    StressTester tester;
    
    // 运行不同规模的并发测试
    std::vector<std::pair<int, int>> testScenarios = {
        {10, 100},    // 10个VM，每个100条指令
        {50, 50},     // 50个VM，每个50条指令
        {100, 20}     // 100个VM，每个20条指令
    };
    
    for (const auto& scenario : testScenarios) {
        tester.runConcurrentVmTest(scenario.first, scenario.second);
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 间隔休息
    }
    
    // 运行长时间测试
    tester.runLongRunningTest(10); // 10秒长时间测试
    
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    Stress Testing Completed" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    return 0;
}