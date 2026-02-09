#ifndef CONSOLE_TERMINAL_H
#define CONSOLE_TERMINAL_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <fstream>
#include <sstream>
#include <functional>
#include "../kernel/CPUvm/baseVM.h"
#include "../kernel/CPUvm/x86Vm.h"
#include "../kernel/CPUvm/armVm.h"
#include "../kernel/CPUvm/x64Vm.h"
#include "../kernel/dispatch/scheduler.h"
#include "../kernel/performance_monitor/performance_monitor.h"

/**
 * @brief 控制台命令结构体
 */
struct S_ConsoleCommand {
    std::string name;           // 命令名称
    std::string description;    // 命令描述
    std::vector<std::string> params; // 参数列表
};

/**
 * @brief VM信息结构体
 */
struct S_VmInfo {
    uint32_t id;
    std::string type;
    std::string status;
    std::string payloadFile;
    std::shared_ptr<I_VmInterface> vmPtr;
};

/**
 * @brief 控制台终端类，提供用户交互界面
 * @details 实现VM管理、调度器控制、性能监控等功能的命令行接口
 */
class ConsoleTerminal {
private:
    bool isRunning;                             // 终端运行状态
    std::map<uint32_t, S_VmInfo> vmRegistry;    // VM注册表
    std::unique_ptr<Scheduler> scheduler;       // 调度器实例
    std::unique_ptr<PerformanceMonitor> perfMonitor; // 性能监控器实例
    uint32_t nextVmId;                          // 下一个VM ID
    
    // 命令映射表
    std::map<std::string, std::function<void(const std::vector<std::string>&)>> commandMap;
    
public:
    ConsoleTerminal();
    ~ConsoleTerminal();
    
    /**
     * @brief 启动控制台终端
     */
    void start();
    
    /**
     * @brief 停止控制台终端
     */
    void stop();
    
    /**
     * @brief 显示欢迎信息
     */
    void showWelcome();
    
    /**
     * @brief 显示帮助信息
     */
    void showHelp();
    
    /**
     * @brief 显示系统状态
     */
    void showStatus();
    
    /**
     * @brief 处理用户输入命令
     * @param input 用户输入的命令行
     */
    void processCommand(const std::string& input);
    
private:
    // 系统命令
    void cmdHelp(const std::vector<std::string>& args);
    void cmdStatus(const std::vector<std::string>& args);
    void cmdExit(const std::vector<std::string>& args);
    
    // VM管理命令
    void cmdVmCreate(const std::vector<std::string>& args);
    void cmdVmList(const std::vector<std::string>& args);
    void cmdVmStart(const std::vector<std::string>& args);
    void cmdVmStop(const std::vector<std::string>& args);
    void cmdVmPause(const std::vector<std::string>& args);
    void cmdVmResume(const std::vector<std::string>& args);
    void cmdVmRun(const std::vector<std::string>& args);
    void cmdVmInfo(const std::vector<std::string>& args);
    void cmdVmDelete(const std::vector<std::string>& args);
    
    // 调度器命令
    void cmdSchedStart(const std::vector<std::string>& args);
    void cmdSchedStop(const std::vector<std::string>& args);
    void cmdSchedAdd(const std::vector<std::string>& args);
    void cmdSchedBind(const std::vector<std::string>& args);
    void cmdSchedUnbind(const std::vector<std::string>& args);
    void cmdSchedStats(const std::vector<std::string>& args);
    
    // 性能监控命令
    void cmdPerfStart(const std::vector<std::string>& args);
    void cmdPerfStop(const std::vector<std::string>& args);
    void cmdPerfReport(const std::vector<std::string>& args);
    
    // 辅助方法
    std::vector<std::string> parseArguments(const std::string& input);
    void registerCommands();
    bool loadPayloadFromFile(const std::string& filename, std::vector<uint8_t>& payload);
    std::shared_ptr<I_VmInterface> createVmInstance(const std::string& type, uint32_t id);
    void printVmInfo(const S_VmInfo& vmInfo);
    void showError(const std::string& error);
    void showSuccess(const std::string& message);
};

/**
 * @brief 自动化测试类
 */
class AutoTestSuite {
private:
    ConsoleTerminal& terminal;
    
public:
    explicit AutoTestSuite(ConsoleTerminal& term) : terminal(term) {}
    
    /**
     * @brief 运行所有自动化测试
     */
    void runAllTests();
    
    /**
     * @brief 基本VM操作测试
     */
    void testBasicVmOperations();
    
    /**
     * @brief 调度器集成测试
     */
    void testSchedulerIntegration();
    
    /**
     * @brief 性能监控测试
     */
    void testPerformanceMonitoring();
    
    /**
     * @brief 压力测试
     */
    void runStressTest();
};

#endif // CONSOLE_TERMINAL_H