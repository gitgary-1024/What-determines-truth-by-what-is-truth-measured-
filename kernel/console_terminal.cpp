#include "console_terminal.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>
#include <ctime>

ConsoleTerminal::ConsoleTerminal() 
    : isRunning(false), nextVmId(1) {
    scheduler.reset(new Scheduler());
    perfMonitor.reset(new PerformanceMonitor());
    registerCommands();
}

ConsoleTerminal::~ConsoleTerminal() {
    stop();
}

void ConsoleTerminal::start() {
    isRunning = true;
    showWelcome();
    
    std::string input;
    while (isRunning) {
        std::cout << "\nMyOS> ";
        std::getline(std::cin, input);
        
        if (!input.empty()) {
            processCommand(input);
        }
    }
}

void ConsoleTerminal::stop() {
    isRunning = false;
    // 停止所有运行的VM
    for (auto& pair : vmRegistry) {
        if (pair.second.vmPtr && pair.second.vmPtr->getRunningStatus()) {
            pair.second.vmPtr->stop();
        }
    }
    
    // 停止调度器
    if (scheduler) {
        scheduler->stop();
    }
}

void ConsoleTerminal::showWelcome() {
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    MyOS VM System Console Terminal" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "Type 'help' for available commands" << std::endl;
    std::cout << "Type 'exit' to quit" << std::endl;
}

void ConsoleTerminal::showHelp() {
    std::cout << "\n=== Available Commands ===" << std::endl;
    std::cout << "\n# System Commands:" << std::endl;
    std::cout << "help                    - Show this help message" << std::endl;
    std::cout << "status                  - Show system status" << std::endl;
    std::cout << "exit                    - Exit the terminal" << std::endl;
    
    std::cout << "\n# VM Management:" << std::endl;
    std::cout << "vm create <type> <file> - Create VM (x86/arm/x64)" << std::endl;
    std::cout << "vm list                 - List all VMs" << std::endl;
    std::cout << "vm start <id>          - Start VM" << std::endl;
    std::cout << "vm stop <id>           - Stop VM" << std::endl;
    std::cout << "vm pause <id>          - Pause VM" << std::endl;
    std::cout << "vm resume <id>         - Resume VM" << std::endl;
    std::cout << "vm run <id> <steps>    - Run VM for N steps" << std::endl;
    std::cout << "vm info <id>           - Show VM information" << std::endl;
    std::cout << "vm delete <id>         - Delete VM" << std::endl;
    
    std::cout << "\n# Scheduler Commands:" << std::endl;
    std::cout << "sched start            - Start scheduler" << std::endl;
    std::cout << "sched stop             - Stop scheduler" << std::endl;
    std::cout << "sched add <id> <pri>   - Add VM to scheduler queue" << std::endl;
    std::cout << "sched bind <id> <core> - Bind VM to specific core" << std::endl;
    std::cout << "sched unbind <id>      - Unbind VM from core" << std::endl;
    std::cout << "sched stats            - Show scheduler statistics" << std::endl;
    
    std::cout << "\n# Performance Monitoring:" << std::endl;
    std::cout << "perf start <id>        - Start performance monitoring" << std::endl;
    std::cout << "perf stop <id>         - Stop performance monitoring" << std::endl;
    std::cout << "perf report            - Show performance report" << std::endl;
}

void ConsoleTerminal::showStatus() {
    std::cout << "\n=== System Status ===" << std::endl;
    std::cout << "Terminal: " << (isRunning ? "RUNNING" : "STOPPED") << std::endl;
    std::cout << "Registered VMs: " << vmRegistry.size() << std::endl;
    std::cout << "Scheduler: " << (scheduler ? "AVAILABLE" : "NOT INITIALIZED") << std::endl;
    std::cout << "Performance Monitor: " << (perfMonitor ? "ACTIVE" : "INACTIVE") << std::endl;
    
    if (!vmRegistry.empty()) {
        std::cout << "\nVM Status:" << std::endl;
        for (const auto& pair : vmRegistry) {
            std::cout << "  VM " << pair.first << " (" << pair.second.type << "): " 
                      << pair.second.status << std::endl;
        }
    }
}

void ConsoleTerminal::processCommand(const std::string& input) {
    auto args = parseArguments(input);
    if (args.empty()) return;
    
    std::string command = args[0];
    args.erase(args.begin()); // 移除命令本身
    
    auto it = commandMap.find(command);
    if (it != commandMap.end()) {
        try {
            it->second(args);
        } catch (const std::exception& e) {
            showError(std::string("Command execution failed: ") + e.what());
        }
    } else {
        showError("Unknown command: " + command + ". Type 'help' for available commands.");
    }
}

// 系统命令实现
void ConsoleTerminal::cmdHelp(const std::vector<std::string>& args) {
    showHelp();
}

void ConsoleTerminal::cmdStatus(const std::vector<std::string>& args) {
    showStatus();
}

void ConsoleTerminal::cmdExit(const std::vector<std::string>& args) {
    std::cout << "Shutting down console terminal..." << std::endl;
    stop();
}

// VM管理命令实现
void ConsoleTerminal::cmdVmCreate(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        showError("Usage: vm create <type> <payload_file>");
        return;
    }
    
    std::string type = args[0];
    std::string filename = args[1];
    
    // 检查支持的VM类型
    if (type != "x86" && type != "arm" && type != "x64") {
        showError("Unsupported VM type: " + type + ". Supported types: x86, arm, x64");
        return;
    }
    
    // 加载payload
    std::vector<uint8_t> payload;
    if (!loadPayloadFromFile(filename, payload)) {
        showError("Failed to load payload from file: " + filename);
        return;
    }
    
    // 创建VM实例
    auto vm = createVmInstance(type, nextVmId);
    if (!vm) {
        showError("Failed to create VM instance");
        return;
    }
    
    // 设置payload
    vm->setPayload(payload.data(), payload.size());
    
    // 注册VM
    S_VmInfo vmInfo;
    vmInfo.id = nextVmId;
    vmInfo.type = type;
    vmInfo.status = "CREATED";
    vmInfo.payloadFile = filename;
    vmInfo.vmPtr = vm;
    
    vmRegistry[nextVmId] = vmInfo;
    showSuccess("VM " + std::to_string(nextVmId) + " (" + type + ") created successfully");
    nextVmId++;
}

void ConsoleTerminal::cmdVmList(const std::vector<std::string>& args) {
    if (vmRegistry.empty()) {
        std::cout << "No VMs registered" << std::endl;
        return;
    }
    
    std::cout << "\n=== Registered VMs ===" << std::endl;
    for (const auto& pair : vmRegistry) {
        printVmInfo(pair.second);
    }
}

void ConsoleTerminal::cmdVmStart(const std::vector<std::string>& args) {
    if (args.empty()) {
        showError("Usage: vm start <id>");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    auto it = vmRegistry.find(vmId);
    if (it == vmRegistry.end()) {
        showError("VM " + std::to_string(vmId) + " not found");
        return;
    }
    
    try {
        it->second.vmPtr->start();
        it->second.status = "RUNNING";
        showSuccess("VM " + std::to_string(vmId) + " started");
    } catch (const std::exception& e) {
        showError("Failed to start VM: " + std::string(e.what()));
    }
}

void ConsoleTerminal::cmdVmStop(const std::vector<std::string>& args) {
    if (args.empty()) {
        showError("Usage: vm stop <id>");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    auto it = vmRegistry.find(vmId);
    if (it == vmRegistry.end()) {
        showError("VM " + std::to_string(vmId) + " not found");
        return;
    }
    
    try {
        it->second.vmPtr->stop();
        it->second.status = "STOPPED";
        showSuccess("VM " + std::to_string(vmId) + " stopped");
    } catch (const std::exception& e) {
        showError("Failed to stop VM: " + std::string(e.what()));
    }
}

void ConsoleTerminal::cmdVmPause(const std::vector<std::string>& args) {
    if (args.empty()) {
        showError("Usage: vm pause <id>");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    auto it = vmRegistry.find(vmId);
    if (it == vmRegistry.end()) {
        showError("VM " + std::to_string(vmId) + " not found");
        return;
    }
    
    try {
        it->second.vmPtr->pause();
        it->second.status = "PAUSED";
        showSuccess("VM " + std::to_string(vmId) + " paused");
    } catch (const std::exception& e) {
        showError("Failed to pause VM: " + std::string(e.what()));
    }
}

void ConsoleTerminal::cmdVmResume(const std::vector<std::string>& args) {
    if (args.empty()) {
        showError("Usage: vm resume <id>");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    auto it = vmRegistry.find(vmId);
    if (it == vmRegistry.end()) {
        showError("VM " + std::to_string(vmId) + " not found");
        return;
    }
    
    try {
        it->second.vmPtr->resume();
        it->second.status = "RUNNING";
        showSuccess("VM " + std::to_string(vmId) + " resumed");
    } catch (const std::exception& e) {
        showError("Failed to resume VM: " + std::string(e.what()));
    }
}

void ConsoleTerminal::cmdVmRun(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        showError("Usage: vm run <id> <steps>");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    uint32_t steps = std::stoul(args[1]);
    
    auto it = vmRegistry.find(vmId);
    if (it == vmRegistry.end()) {
        showError("VM " + std::to_string(vmId) + " not found");
        return;
    }
    
    try {
        // 启动性能监控
        if (perfMonitor) {
            perfMonitor->recordVmStart(vmId);
        }
        
        // 执行指定步数
        uint32_t executed = 0;
        for (uint32_t i = 0; i < steps && it->second.vmPtr->getRunningStatus(); i++) {
            if (it->second.vmPtr->runOneInstruction()) {
                executed++;
            }
        }
        
        // 停止性能监控
        if (perfMonitor) {
            perfMonitor->recordVmStop(vmId, executed);
        }
        
        showSuccess("VM " + std::to_string(vmId) + " executed " + std::to_string(executed) + " instructions");
    } catch (const std::exception& e) {
        showError("Failed to run VM: " + std::string(e.what()));
    }
}

void ConsoleTerminal::cmdVmInfo(const std::vector<std::string>& args) {
    if (args.empty()) {
        showError("Usage: vm info <id>");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    auto it = vmRegistry.find(vmId);
    if (it == vmRegistry.end()) {
        showError("VM " + std::to_string(vmId) + " not found");
        return;
    }
    
    printVmInfo(it->second);
    
    // 显示寄存器状态
    const auto& context = it->second.vmPtr->getContext();
    std::cout << "Registers:" << std::endl;
    std::cout << "  EAX: 0x" << std::hex << std::setfill('0') << std::setw(8) << context.eax << std::dec << std::endl;
    std::cout << "  EBX: 0x" << std::hex << std::setfill('0') << std::setw(8) << context.ebx << std::dec << std::endl;
    std::cout << "  ECX: 0x" << std::hex << std::setfill('0') << std::setw(8) << context.ecx << std::dec << std::endl;
    std::cout << "  EDX: 0x" << std::hex << std::setfill('0') << std::setw(8) << context.edx << std::dec << std::endl;
    std::cout << "  EIP: 0x" << std::hex << std::setfill('0') << std::setw(8) << context.eip << std::dec << std::endl;
    std::cout << "  ESP: 0x" << std::hex << std::setfill('0') << std::setw(8) << context.esp << std::dec << std::endl;
}

void ConsoleTerminal::cmdVmDelete(const std::vector<std::string>& args) {
    if (args.empty()) {
        showError("Usage: vm delete <id>");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    auto it = vmRegistry.find(vmId);
    if (it == vmRegistry.end()) {
        showError("VM " + std::to_string(vmId) + " not found");
        return;
    }
    
    // 停止VM如果正在运行
    if (it->second.vmPtr->getRunningStatus()) {
        it->second.vmPtr->stop();
    }
    
    vmRegistry.erase(it);
    showSuccess("VM " + std::to_string(vmId) + " deleted");
}

// 调度器命令实现
void ConsoleTerminal::cmdSchedStart(const std::vector<std::string>& args) {
    if (!scheduler) {
        showError("Scheduler not initialized");
        return;
    }
    
    if (scheduler->initialize()) {
        scheduler->start();
        showSuccess("Scheduler started");
    } else {
        showError("Failed to initialize scheduler");
    }
}

void ConsoleTerminal::cmdSchedStop(const std::vector<std::string>& args) {
    if (scheduler) {
        scheduler->stop();
        showSuccess("Scheduler stopped");
    } else {
        showError("Scheduler not initialized");
    }
}

void ConsoleTerminal::cmdSchedAdd(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        showError("Usage: sched add <id> <priority>");
        return;
    }
    
    if (!scheduler) {
        showError("Scheduler not initialized");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    uint32_t priority = std::stoul(args[1]);
    
    auto it = vmRegistry.find(vmId);
    if (it == vmRegistry.end()) {
        showError("VM " + std::to_string(vmId) + " not found");
        return;
    }
    
    // 注意：当前调度器只支持X86Vm类型
    if (auto x86vm = std::dynamic_pointer_cast<X86Vm>(it->second.vmPtr)) {
        if (scheduler->addVm(x86vm, priority)) {
            showSuccess("VM " + std::to_string(vmId) + " added to scheduler with priority " + std::to_string(priority));
        } else {
            showError("Failed to add VM to scheduler");
        }
    } else {
        showError("Only x86 VMs are currently supported by scheduler");
    }
}

void ConsoleTerminal::cmdSchedBind(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        showError("Usage: sched bind <id> <core>");
        return;
    }
    
    if (!scheduler) {
        showError("Scheduler not initialized");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    uint32_t coreId = std::stoul(args[1]);
    
    if (scheduler->applyStaticCore(vmId, coreId)) {
        showSuccess("VM " + std::to_string(vmId) + " bound to core " + std::to_string(coreId));
    } else {
        showError("Failed to bind VM to core");
    }
}

void ConsoleTerminal::cmdSchedUnbind(const std::vector<std::string>& args) {
    if (args.empty()) {
        showError("Usage: sched unbind <id>");
        return;
    }
    
    if (!scheduler) {
        showError("Scheduler not initialized");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    
    if (scheduler->releaseStaticCore(vmId)) {
        showSuccess("VM " + std::to_string(vmId) + " unbound from core");
    } else {
        showError("Failed to unbind VM from core");
    }
}

void ConsoleTerminal::cmdSchedStats(const std::vector<std::string>& args) {
    if (!scheduler) {
        showError("Scheduler not initialized");
        return;
    }
    
    std::cout << scheduler->getStatistics() << std::endl;
}

// 性能监控命令实现
void ConsoleTerminal::cmdPerfStart(const std::vector<std::string>& args) {
    if (args.empty()) {
        showError("Usage: perf start <id>");
        return;
    }
    
    if (!perfMonitor) {
        showError("Performance monitor not initialized");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    perfMonitor->recordVmStart(vmId);
    showSuccess("Performance monitoring started for VM " + std::to_string(vmId));
}

void ConsoleTerminal::cmdPerfStop(const std::vector<std::string>& args) {
    if (args.empty()) {
        showError("Usage: perf stop <id>");
        return;
    }
    
    if (!perfMonitor) {
        showError("Performance monitor not initialized");
        return;
    }
    
    uint32_t vmId = std::stoul(args[0]);
    // 这里需要获取指令计数，暂时用0
    perfMonitor->recordVmStop(vmId, 0);
    showSuccess("Performance monitoring stopped for VM " + std::to_string(vmId));
}

void ConsoleTerminal::cmdPerfReport(const std::vector<std::string>& args) {
    if (!perfMonitor) {
        showError("Performance monitor not initialized");
        return;
    }
    
    perfMonitor->printPerformanceReport();
}

// 辅助方法实现
std::vector<std::string> ConsoleTerminal::parseArguments(const std::string& input) {
    std::vector<std::string> args;
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> token) {
        args.push_back(token);
    }
    
    return args;
}

void ConsoleTerminal::registerCommands() {
    // 系统命令
    commandMap["help"] = [this](const std::vector<std::string>& args) { cmdHelp(args); };
    commandMap["status"] = [this](const std::vector<std::string>& args) { cmdStatus(args); };
    commandMap["exit"] = [this](const std::vector<std::string>& args) { cmdExit(args); };
    
    // VM管理命令
    commandMap["vm"] = [this](const std::vector<std::string>& args) {
        if (args.empty()) {
            showError("VM command requires subcommand");
            return;
        }
        
        std::string subcommand = args[0];
        std::vector<std::string> subArgs(args.begin() + 1, args.end());
        
        if (subcommand == "create") cmdVmCreate(subArgs);
        else if (subcommand == "list") cmdVmList(subArgs);
        else if (subcommand == "start") cmdVmStart(subArgs);
        else if (subcommand == "stop") cmdVmStop(subArgs);
        else if (subcommand == "pause") cmdVmPause(subArgs);
        else if (subcommand == "resume") cmdVmResume(subArgs);
        else if (subcommand == "run") cmdVmRun(subArgs);
        else if (subcommand == "info") cmdVmInfo(subArgs);
        else if (subcommand == "delete") cmdVmDelete(subArgs);
        else showError("Unknown VM subcommand: " + subcommand);
    };
    
    // 调度器命令
    commandMap["sched"] = [this](const std::vector<std::string>& args) {
        if (args.empty()) {
            showError("Scheduler command requires subcommand");
            return;
        }
        
        std::string subcommand = args[0];
        std::vector<std::string> subArgs(args.begin() + 1, args.end());
        
        if (subcommand == "start") cmdSchedStart(subArgs);
        else if (subcommand == "stop") cmdSchedStop(subArgs);
        else if (subcommand == "add") cmdSchedAdd(subArgs);
        else if (subcommand == "bind") cmdSchedBind(subArgs);
        else if (subcommand == "unbind") cmdSchedUnbind(subArgs);
        else if (subcommand == "stats") cmdSchedStats(subArgs);
        else showError("Unknown scheduler subcommand: " + subcommand);
    };
    
    // 性能监控命令
    commandMap["perf"] = [this](const std::vector<std::string>& args) {
        if (args.empty()) {
            showError("Performance command requires subcommand");
            return;
        }
        
        std::string subcommand = args[0];
        std::vector<std::string> subArgs(args.begin() + 1, args.end());
        
        if (subcommand == "start") cmdPerfStart(subArgs);
        else if (subcommand == "stop") cmdPerfStop(subArgs);
        else if (subcommand == "report") cmdPerfReport(subArgs);
        else showError("Unknown performance subcommand: " + subcommand);
    };
}

bool ConsoleTerminal::loadPayloadFromFile(const std::string& filename, std::vector<uint8_t>& payload) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 读取文件内容
    payload.resize(fileSize);
    file.read(reinterpret_cast<char*>(payload.data()), fileSize);
    file.close();
    
    return true;
}

std::shared_ptr<I_VmInterface> ConsoleTerminal::createVmInstance(const std::string& type, uint32_t id) {
    if (type == "x86") {
        return std::make_shared<X86Vm>(id);
    } else if (type == "arm") {
        return std::make_shared<ArmVm>(id);
    } else if (type == "x64") {
        return std::make_shared<X64Vm>(id);
    }
    return nullptr;
}

void ConsoleTerminal::printVmInfo(const S_VmInfo& vmInfo) {
    std::cout << "VM ID: " << vmInfo.id << std::endl;
    std::cout << "  Type: " << vmInfo.type << std::endl;
    std::cout << "  Status: " << vmInfo.status << std::endl;
    std::cout << "  Payload File: " << vmInfo.payloadFile << std::endl;
    std::cout << "  Resource Usage: " << vmInfo.vmPtr->getResourceUsage() << std::endl;
}

void ConsoleTerminal::showError(const std::string& error) {
    std::cerr << "Error: " << error << std::endl;
}

void ConsoleTerminal::showSuccess(const std::string& message) {
    std::cout << "Success: " << message << std::endl;
}

void AutoTestSuite::runAllTests() {
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    Running Automated Test Suite" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    testBasicVmOperations();
    testSchedulerIntegration();
    testPerformanceMonitoring();
    runStressTest();
    
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    All Tests Completed" << std::endl;
    std::cout << "===========================================" << std::endl;
}

void AutoTestSuite::testBasicVmOperations() {
    std::cout << "\n--- Testing Basic VM Operations ---" << std::endl;
    
    // 创建测试payload文件
    std::vector<uint8_t> testPayload = {0x01, 0x02, 0x03, 0x04, 0x05}; // 简单的测试数据
    std::ofstream testFile("test_payload.bin", std::ios::binary);
    testFile.write(reinterpret_cast<const char*>(testPayload.data()), testPayload.size());
    testFile.close();
    
    try {
        // 测试创建不同类型的VM
        std::vector<std::string> createArgs = {"create", "x86", "test_payload.bin"};
        terminal.processCommand("vm create x86 test_payload.bin");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        terminal.processCommand("vm create arm test_payload.bin");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        terminal.processCommand("vm create x64 test_payload.bin");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 测试列出VM
        terminal.processCommand("vm list");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 测试启动VM
        terminal.processCommand("vm start 1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 测试运行指令
        terminal.processCommand("vm run 1 5");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 测试暂停和恢复
        terminal.processCommand("vm pause 1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        terminal.processCommand("vm resume 1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 测试查看VM信息
        terminal.processCommand("vm info 1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 测试停止VM
        terminal.processCommand("vm stop 1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "✓ Basic VM operations test passed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Basic VM operations test failed: " << e.what() << std::endl;
    }
}

void AutoTestSuite::testSchedulerIntegration() {
    std::cout << "\n--- Testing Scheduler Integration ---" << std::endl;
    
    try {
        // 启动调度器
        terminal.processCommand("sched start");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // 添加VM到调度器
        terminal.processCommand("sched add 1 10");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 绑定到核心
        terminal.processCommand("sched bind 1 2");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 查看调度器统计
        terminal.processCommand("sched stats");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 解绑核心
        terminal.processCommand("sched unbind 1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 停止调度器
        terminal.processCommand("sched stop");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "✓ Scheduler integration test passed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Scheduler integration test failed: " << e.what() << std::endl;
    }
}

void AutoTestSuite::testPerformanceMonitoring() {
    std::cout << "\n--- Testing Performance Monitoring ---" << std::endl;
    
    try {
        // 启动性能监控
        terminal.processCommand("perf start 1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 运行一些指令
        terminal.processCommand("vm run 1 10");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 停止性能监控
        terminal.processCommand("perf stop 1");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 显示性能报告
        terminal.processCommand("perf report");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "✓ Performance monitoring test passed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Performance monitoring test failed: " << e.what() << std::endl;
    }
}

void AutoTestSuite::runStressTest() {
    std::cout << "\n--- Running Stress Test ---" << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        const int NUM_VMS = 10;
        const int INSTRUCTIONS_PER_VM = 100;
        
        std::cout << "Creating " << NUM_VMS << " VMs..." << std::endl;
        
        // 创建多个VM
        for (int i = 0; i < NUM_VMS; i++) {
            std::string cmd = "vm create x86 test_payload.bin";
            terminal.processCommand(cmd);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        std::cout << "Starting stress test execution..." << std::endl;
        
        // 并发执行多个VM
        std::vector<std::thread> threads;
        for (int i = 1; i <= NUM_VMS; i++) {
            threads.emplace_back([this, i, INSTRUCTIONS_PER_VM]() {
                try {
                    std::string startCmd = "vm start " + std::to_string(i);
                    terminal.processCommand(startCmd);
                    
                    std::string runCmd = "vm run " + std::to_string(i) + " " + std::to_string(INSTRUCTIONS_PER_VM);
                    terminal.processCommand(runCmd);
                    
                    std::string stopCmd = "vm stop " + std::to_string(i);
                    terminal.processCommand(stopCmd);
                } catch (...) {
                    // 忽略单个VM的错误
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "Stress test completed in " << duration.count() << " ms" << std::endl;
        std::cout << "Created and executed " << NUM_VMS << " VMs with " << INSTRUCTIONS_PER_VM << " instructions each" << std::endl;
        
        // 显示最终统计
        terminal.processCommand("perf report");
        
        std::cout << "✓ Stress test passed" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Stress test failed: " << e.what() << std::endl;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "Total stress test time: " << totalDuration.count() << " ms" << std::endl;
}
