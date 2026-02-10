#include <iostream>
#include <vector>
#include <fstream>
#include <memory>
#include "kernel/CPUvm/armVm.h"
#include "kernel/CPUvm/x64Vm.h"
#include <stdint.h>

/**
 * @brief ARM分支指令BUG复现程序
 * 这个程序专门用来演示和复现ARM VM中分支指令的符号扩展错误
 */

void demonstrateArmBranchBug() {
    std::cout << "===========================================" << std::endl;
    std::cout << "    ARM Branch Instruction Bug Demo" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    // 创建ARM VM实例（小端模式）
    std::shared_ptr<ArmVm> armVm = std::make_shared<ArmVm>(1, false);
    
    // 创建包含分支指令的payload
    // ARM B指令格式: cond(4) 101 10 offset(24)
    // 我们创建一个向前跳转的分支指令
    std::vector<uint8_t> branchPayload = {
        // 指令1: mov r0, #1 (0xE3A00001)
        0x01, 0x00, 0xA0, 0xE3,
        // 指令2: B +8字节 (跳过接下来的2条指令) 
        // B指令: 1110 1010 offset(24位)
        // offset = 8字节 / 4 = 2条指令 = 0x000002
        // 但是由于ARM指令是24位有符号立即数，我们需要正确计算
        0x02, 0x00, 0x00, 0xEA,  // EA 00 00 02 (小端)
        // 指令3: mov r1, #2 (这条应该被跳过)
        0x02, 0x10, 0xA0, 0xE3,
        // 指令4: mov r2, #3 (这条应该被执行)
        0x03, 0x20, 0xA0, 0xE3,
        // 指令5: mov r3, #4 (这条应该被执行)
        0x04, 0x30, 0xA0, 0xE3
    };
    
    std::cout << "Original payload size: " << branchPayload.size() << " bytes" << std::endl;
    std::cout << "Expected execution flow:" << std::endl;
    std::cout << "1. mov r0, #1     <- executed" << std::endl;
    std::cout << "2. B +8 bytes     <- executed (should jump to instruction 5)" << std::endl;
    std::cout << "3. mov r1, #2     <- SKIPPED due to branch" << std::endl;
    std::cout << "4. mov r2, #3     <- SKIPPED due to branch" << std::endl;
    std::cout << "5. mov r3, #4     <- executed (target of branch)" << std::endl;
    
    // 设置payload
    armVm->setPayload(branchPayload.data(), branchPayload.size());
    
    std::cout << "\n--- Current ARM VM Branch Implementation ---" << std::endl;
    std::cout << "Current buggy code: pc += (static_cast<int32_t>(operand2 << 8) >> 8) * 4;" << std::endl;
    std::cout << "Problem: operand2 only contains 12 bits, not the full 24-bit offset!" << std::endl;
    
    // 手动分析指令来展示问题
    uint32_t instruction = 0xEA000002; // B指令的小端表示
    uint32_t opcode = (instruction >> 21) & 0xF;  // 应该是0xE
    uint32_t operand2 = instruction & 0xFFF;      // 只提取了低12位!
    
    std::cout << "\nInstruction analysis:" << std::endl;
    std::cout << "Full instruction: 0x" << std::hex << instruction << std::dec << std::endl;
    std::cout << "Opcode (bits 21-24): 0x" << std::hex << opcode << std::dec << std::endl;
    std::cout << "Operand2 (bits 0-11): 0x" << std::hex << operand2 << std::dec << std::endl;
    std::cout << "Current buggy calculation: " << (static_cast<int32_t>(operand2 << 8) >> 8) * 4 << " bytes" << std::endl;
    
    // 正确的计算应该是
    int32_t correctOffset = static_cast<int32_t>(instruction << 8) >> 8;  // 符号扩展24位
    int32_t correctByteOffset = correctOffset << 2;  // 转换为字节偏移
    
    std::cout << "Correct calculation:" << std::endl;
    std::cout << "24-bit offset (sign extended): " << correctOffset << std::endl;
    std::cout << "Byte offset: " << correctByteOffset << " bytes" << std::endl;
    
    std::cout << "\n--- Actual Execution Demonstration ---" << std::endl;
    
    try {
        armVm->start();
        
        // 执行指令直到完成
        int executed = 0;
        while (armVm->runOneInstruction()) {
            executed++;
            if (executed > 10) {  // 防止无限循环
                std::cout << "Breaking after 10 instructions to prevent infinite loop" << std::endl;
                break;
            }
        }
        
        armVm->stop();
        
        std::cout << "\nExecution completed with " << executed << " instructions executed" << std::endl;
        
        // 显示最终寄存器状态
        const auto& context = armVm->getContext();
        std::cout << "Final register states:" << std::endl;
        std::cout << "r0: " << context.eax << " (should be 1)" << std::endl;
        std::cout << "r1: " << context.ebx << " (should be 0 if branch worked correctly)" << std::endl;
        std::cout << "r2: " << context.ecx << " (should be 0 if branch worked correctly)" << std::endl;
        std::cout << "r3: " << context.edx << " (should be 4 if branch worked correctly)" << std::endl;
        std::cout << "pc: " << context.eip << " (final program counter)" << std::endl;
        
        // 判断BUG是否触发
        if (context.ebx != 0 || context.ecx != 0) {
            std::cout << "\n🚨 BUG CONFIRMED: Branch instruction did not work correctly!" << std::endl;
            std::cout << "r1 and r2 should be 0 (skipped by branch), but they have values!" << std::endl;
        } else if (context.edx == 4) {
            std::cout << "\n✅ Branch worked correctly - r3 has expected value 4" << std::endl;
        } else {
            std::cout << "\n⚠️  Unclear result - need more detailed analysis" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Error during execution: " << e.what() << std::endl;
    }
}

void demonstrateX64ContextBug() {
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    x64 Context Mapping Bug Demo" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    // 创建x64 VM实例
    std::shared_ptr<X64Vm> x64Vm = std::make_shared<X64Vm>(2);
    
    std::cout << "x64 VM context mapping issue:" << std::endl;
    std::cout << "- saveContext(): 64-bit registers -> 32-bit context (truncation)" << std::endl;
    std::cout << "- loadContext(): 32-bit context -> 64-bit registers (zero extension)" << std::endl;
    std::cout << "- Problem: High 32 bits are lost during save/load cycle!" << std::endl;
    
    // 设置一些64位值来演示问题
    x64Vm->setRegister64("rax", 0x123456789ABCDEF0ULL);
    x64Vm->setRegister64("rbx", 0xFEDCBA9876543210ULL);
    
    std::cout << "\nInitial 64-bit register values:" << std::endl;
    std::cout << "RAX: 0x" << std::hex << x64Vm->getRegister64("rax") << std::dec << std::endl;
    std::cout << "RBX: 0x" << std::hex << x64Vm->getRegister64("rbx") << std::dec << std::endl;
    
    // 保存上下文
    x64Vm->saveContext();
    
    std::cout << "\nAfter saveContext() - truncated to 32-bit:" << std::endl;
    const auto& context = x64Vm->getContext();
    std::cout << "context.eax: 0x" << std::hex << context.eax << std::dec << " (low 32 bits of RAX)" << std::endl;
    std::cout << "context.ebx: 0x" << std::hex << context.ebx << std::dec << " (high 32 bits of RAX)" << std::endl;
    std::cout << "context.ecx: 0x" << std::hex << context.ecx << std::dec << " (low 32 bits of RBX)" << std::endl;
    std::cout << "context.edx: 0x" << std::hex << context.edx << std::dec << " (high 32 bits of RBX)" << std::endl;
    
    // 修改上下文中的值
    // 这里模拟上下文被外部修改的情况
    auto modifiedContext = context;
    modifiedContext.eax = 0x11111111;
    modifiedContext.ebx = 0x22222222;
    
    // 恢复上下文（这里模拟loadContext的行为）
    x64Vm->loadContext();
    
    std::cout << "\nAfter loadContext() - zero extended back to 64-bit:" << std::endl;
    std::cout << "RAX: 0x" << std::hex << x64Vm->getRegister64("rax") << std::dec << std::endl;
    std::cout << "RBX: 0x" << std::hex << x64Vm->getRegister64("rbx") << std::dec << std::endl;
    
    // 检查高32位是否丢失
    uint64_t highBitsLost = (x64Vm->getRegister64("rax") >> 32) == 0 &&
                           (x64Vm->getRegister64("rbx") >> 32) == 0;
    
    if (highBitsLost) {
        std::cout << "\n🚨 CONTEXT BUG CONFIRMED: High 32 bits were lost!" << std::endl;
        std::cout << "Original high bits contained meaningful data that is now gone." << std::endl;
    } else {
        std::cout << "\n✅ Context mapping appears to work correctly" << std::endl;
    }
}

int main() {
    std::cout << "MyOS VM System - BUG Demonstration Program" << std::endl;
    std::cout << "This program demonstrates two critical bugs in the VM implementation" << std::endl;
    
    demonstrateArmBranchBug();
    demonstrateX64ContextBug();
    
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    Summary" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "1. ARM Branch Instruction Bug:" << std::endl;
    std::cout << "   - Wrong operand extraction from instruction" << std::endl;
    std::cout << "   - Incorrect sign extension logic" << std::endl;
    std::cout << "   - Causes incorrect branching behavior" << std::endl;
    std::cout << std::endl;
    std::cout << "2. x64 Context Mapping Bug:" << std::endl;
    std::cout << "   - Loss of high 32 bits during save/load" << std::endl;
    std::cout << "   - Data corruption in 64-bit registers" << std::endl;
    std::cout << "   - Affects all 64-bit operations" << std::endl;
    std::cout << std::endl;
    std::cout << "Both bugs can cause unpredictable VM behavior and crashes!" << std::endl;
    
    return 0;
}