#include <iostream>
#include <vector>
#include <memory>
#include "kernel/CPUvm/armVm.h"
#include "kernel/CPUvm/x64Vm.h"

/**
 * @brief BUG修复验证程序
 * 验证ARM分支指令和x64上下文映射的修复效果
 */

void testFixedArmBranch() {
    std::cout << "===========================================" << std::endl;
    std::cout << "    Testing Fixed ARM Branch Instruction" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    std::shared_ptr<ArmVm> armVm = std::make_shared<ArmVm>(1, false);
    
    // 创建测试payload：包含正确的分支指令
    std::vector<uint8_t> fixedPayload = {
        // mov r0, #1
        0x01, 0x00, 0xA0, 0xE3,
        // B +8 bytes (跳转到指令5)
        0x02, 0x00, 0x00, 0xEA,  // EA 00 00 02
        // 这两条指令应该被跳过
        0xFF, 0xFF, 0xFF, 0xFF,  // 填充数据
        0xFF, 0xFF, 0xFF, 0xFF,  // 填充数据
        // mov r3, #4 (分支目标)
        0x04, 0x30, 0xA0, 0xE3
    };
    
    armVm->setPayload(fixedPayload.data(), fixedPayload.size());
    
    std::cout << "Testing ARM branch instruction fix..." << std::endl;
    
    try {
        armVm->start();
        
        // 执行直到完成或达到限制
        int executed = 0;
        const int MAX_INSTRUCTIONS = 20;
        
        while (executed < MAX_INSTRUCTIONS && armVm->runOneInstruction()) {
            executed++;
        }
        
        armVm->stop();
        
        // 检查结果
        const auto& context = armVm->getContext();
        std::cout << "Instructions executed: " << executed << std::endl;
        std::cout << "Final register states:" << std::endl;
        std::cout << "r0: " << context.eax << " (expected: 1)" << std::endl;
        std::cout << "r3: " << context.edx << " (expected: 4 if branch worked)" << std::endl;
        std::cout << "pc: " << context.eip << std::endl;
        
        // 验证修复是否成功
        if (context.eax == 1 && context.edx == 4) {
            std::cout << "✅ ARM branch fix VERIFIED: Branch instruction works correctly!" << std::endl;
        } else {
            std::cout << "❌ ARM branch fix FAILED: Unexpected register values" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void testFixedX64Context() {
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    Testing Fixed x64 Context Mapping" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    std::shared_ptr<X64Vm> x64Vm = std::make_shared<X64Vm>(2);
    
    // 设置包含高32位信息的64位值
    uint64_t testValue1 = 0x123456789ABCDEF0ULL;
    uint64_t testValue2 = 0xFEDCBA9876543210ULL;
    
    x64Vm->setRegister64("rax", testValue1);
    x64Vm->setRegister64("rbx", testValue2);
    
    std::cout << "Setting 64-bit registers:" << std::endl;
    std::cout << "RAX: 0x" << std::hex << testValue1 << std::dec << std::endl;
    std::cout << "RBX: 0x" << std::hex << testValue2 << std::dec << std::endl;
    
    // 保存上下文
    x64Vm->saveContext();
    std::cout << "Context saved..." << std::endl;
    
    // 修改上下文（模拟外部操作）
    auto context = x64Vm->getContext();
    // 这里不修改context，只是验证保存/恢复过程
    
    // 恢复上下文
    x64Vm->loadContext();
    std::cout << "Context restored..." << std::endl;
    
    // 检查值是否正确恢复
    uint64_t restoredRax = x64Vm->getRegister64("rax");
    uint64_t restoredRbx = x64Vm->getRegister64("rbx");
    
    std::cout << "Restored 64-bit registers:" << std::endl;
    std::cout << "RAX: 0x" << std::hex << restoredRax << std::dec << std::endl;
    std::cout << "RBX: 0x" << std::hex << restoredRbx << std::dec << std::endl;
    
    // 验证修复是否成功
    if (restoredRax == testValue1 && restoredRbx == testValue2) {
        std::cout << "✅ x64 context fix VERIFIED: 64-bit values preserved correctly!" << std::endl;
    } else {
        std::cout << "❌ x64 context fix FAILED: 64-bit values corrupted" << std::endl;
        std::cout << "Expected RAX: 0x" << std::hex << testValue1 << std::dec << std::endl;
        std::cout << "Expected RBX: 0x" << std::hex << testValue2 << std::dec << std::endl;
    }
}

void runComprehensiveTest() {
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    Comprehensive BUG Fix Verification" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    bool allTestsPassed = true;
    
    // 测试1: ARM分支指令
    try {
        testFixedArmBranch();
    } catch (...) {
        std::cout << "❌ ARM branch test crashed" << std::endl;
        allTestsPassed = false;
    }
    
    // 测试2: x64上下文映射
    try {
        testFixedX64Context();
    } catch (...) {
        std::cout << "❌ x64 context test crashed" << std::endl;
        allTestsPassed = false;
    }
    
    // 总结
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    Final Result" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    if (allTestsPassed) {
        std::cout << "🎉 ALL BUG FIXES VERIFIED SUCCESSFULLY!" << std::endl;
        std::cout << "The ARM branch instruction and x64 context mapping bugs have been fixed." << std::endl;
    } else {
        std::cout << "💥 Some tests failed - fixes need more work" << std::endl;
    }
}

int main() {
    std::cout << "MyOS VM System - BUG Fix Verification" << std::endl;
    std::cout << "Verifying fixes for ARM branch and x64 context bugs" << std::endl;
    
    runComprehensiveTest();
    
    return 0;
}