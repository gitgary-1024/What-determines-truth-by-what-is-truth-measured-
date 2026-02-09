#include <iostream>
#include <memory>
#include <string>
#include "kernel/console_terminal.h"

/**
 * @brief 生成测试payload文件
 */
void generateTestPayloads() {
    std::cout << "Generating test payload files..." << std::endl;
    
    // 生成x86测试payload (简单的加法循环)
    std::vector<uint8_t> x86Payload = {
        0xB8, 0x01, 0x00, 0x00, 0x00,  // mov eax, 1
        0x05, 0x01, 0x00, 0x00, 0x00,  // add eax, 1
        0x40,                          // inc eax
        0x48,                          // dec eax
        0x90,                          // nop
        0xEB, 0xFA                     // jmp -6 (无限循环)
    };
    
    std::ofstream x86File("x86_test.bin", std::ios::binary);
    x86File.write(reinterpret_cast<const char*>(x86Payload.data()), x86Payload.size());
    x86File.close();
    
    // 生成ARM测试payload (ARM指令示例)
    std::vector<uint8_t> armPayload = {
        0x01, 0x00, 0xA0, 0xE3,  // mov r0, #1
        0x01, 0x00, 0x80, 0xE2,  // add r0, r0, #1
        0x01, 0x00, 0x50, 0xE3,  // cmp r0, #1
        0xFC, 0xFF, 0xFF, 0xEA   // b -4 (无限循环)
    };
    
    std::ofstream armFile("arm_test.bin", std::ios::binary);
    armFile.write(reinterpret_cast<const char*>(armPayload.data()), armPayload.size());
    armFile.close();
    
    // 生成x64测试payload (x64指令示例)
    std::vector<uint8_t> x64Payload = {
        0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00,  // mov rax, 1
        0x48, 0xFF, 0xC0,                          // inc rax
        0x48, 0xFF, 0xC8,                          // dec rax
        0xEB, 0xF6                                 // jmp -10 (无限循环)
    };
    
    std::ofstream x64File("x64_test.bin", std::ios::binary);
    x64File.write(reinterpret_cast<const char*>(x64Payload.data()), x64Payload.size());
    x64File.close();
    
    std::cout << "Test payload files generated successfully!" << std::endl;
    std::cout << "- x86_test.bin" << std::endl;
    std::cout << "- arm_test.bin" << std::endl;
    std::cout << "- x64_test.bin" << std::endl;
}

/**
 * @brief 显示系统信息
 */
void showSystemInfo() {
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    MyOS VM System v1.0" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "Supported Architectures: x86, ARM, x64" << std::endl;
    std::cout << "Features: VM Management, Scheduling, Performance Monitoring" << std::endl;
    std::cout << "===========================================" << std::endl;
}

/**
 * @brief 运行交互式控制台
 */
void runInteractiveConsole() {
    ConsoleTerminal terminal;
    terminal.start();
}

/**
 * @brief 运行自动化测试
 */
void runAutomatedTests() {
    ConsoleTerminal terminal;
    AutoTestSuite testSuite(terminal);
    testSuite.runAllTests();
}

int main() {
    try {
        showSystemInfo();
        
        // 生成测试文件
        generateTestPayloads();
        
        // 询问用户选择模式
        std::cout << "\nSelect operation mode:" << std::endl;
        std::cout << "1. Interactive Console Terminal" << std::endl;
        std::cout << "2. Automated Test Suite" << std::endl;
        std::cout << "Enter choice (1 or 2): ";
        
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "1") {
            std::cout << "\nStarting Interactive Console Terminal..." << std::endl;
            runInteractiveConsole();
        } else if (choice == "2") {
            std::cout << "\nStarting Automated Test Suite..." << std::endl;
            runAutomatedTests();
        } else {
            std::cout << "Invalid choice. Starting Interactive Console Terminal..." << std::endl;
            runInteractiveConsole();
        }
        
        std::cout << "\nProgram terminated normally." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}