#ifndef X64_VM_H
#define X64_VM_H

#include "baseVM.h"
#include <iostream>
#include <stdexcept>
#include <cstdint>

/**
 * @brief x64 VM实现类，模拟x86-64架构的基本功能
 * @details 实现64位寄存器和扩展指令集，支持64位地址空间
 */
class X64Vm : public I_VmInterface {
private:
    // x64 64位寄存器
    uint64_t rax, rbx, rcx, rdx;     // 64位通用寄存器
    uint64_t rsi, rdi, rbp, rsp;     // 64位指针寄存器
    uint64_t r8, r9, r10, r11;       // 扩展通用寄存器
    uint64_t r12, r13, r14, r15;     // 扩展通用寄存器
    uint64_t rip;                    // 64位指令指针
    uint64_t rflags;                 // 64位标志寄存器
    
    // x64标志位掩码（使用低32位）
    static const uint64_t FLAG_CF = 1ULL << 0;   // 进位标志
    static const uint64_t FLAG_PF = 1ULL << 2;   // 奇偶标志
    static const uint64_t FLAG_AF = 1ULL << 4;   // 辅助进位标志
    static const uint64_t FLAG_ZF = 1ULL << 6;   // 零标志
    static const uint64_t FLAG_SF = 1ULL << 7;   // 符号标志
    static const uint64_t FLAG_OF = 1ULL << 11;  // 溢出标志
    
    uint32_t resourceLimit;     // 资源限制
    uint32_t instructionCount;  // 已执行指令计数
    
public:
    /**
     * @brief 构造函数
     * @param id VM唯一标识符
     */
    explicit X64Vm(uint32_t id) 
        : I_VmInterface(id), 
          rax(0), rbx(0), rcx(0), rdx(0), rsi(0), rdi(0), rbp(0), rsp(0),
          r8(0), r9(0), r10(0), r11(0), r12(0), r13(0), r14(0), r15(0),
          rip(0), rflags(0),
          resourceLimit(10000), instructionCount(0) {}
    
    // 实现基类的纯虚函数
    void start() override {
        if (isRunning) {
            throw std::runtime_error("VM already running");
        }
        isRunning = true;
        std::cout << "x64 VM " << vmId << " started" << std::endl;
    }
    
    void pause() override {
        if (!isRunning) {
            throw std::runtime_error("VM not running");
        }
        isRunning = false;
        saveContext();
        std::cout << "x64 VM " << vmId << " paused" << std::endl;
    }
    
    void resume() override {
        if (isRunning) {
            throw std::runtime_error("VM already running");
        }
        loadContext();
        isRunning = true;
        std::cout << "x64 VM " << vmId << " resumed" << std::endl;
    }
    
    void stop() override {
        isRunning = false;
        std::cout << "x64 VM " << vmId << " stopped normally" << std::endl;
    }
    
    void forceStop() override {
        isRunning = false;
        std::cout << "x64 VM " << vmId << " force stopped" << std::endl;
    }
    
    void saveContext() override {
        // 将64位寄存器状态映射到32位context结构体（截断高32位）
        context.eax = static_cast<uint32_t>(rax & 0xFFFFFFFF);
        context.ebx = static_cast<uint32_t>(rbx & 0xFFFFFFFF);
        context.ecx = static_cast<uint32_t>(rcx & 0xFFFFFFFF);
        context.edx = static_cast<uint32_t>(rdx & 0xFFFFFFFF);
        context.esi = static_cast<uint32_t>(rsi & 0xFFFFFFFF);
        context.edi = static_cast<uint32_t>(rdi & 0xFFFFFFFF);
        context.ebp = static_cast<uint32_t>(rbp & 0xFFFFFFFF);
        context.esp = static_cast<uint32_t>(rsp & 0xFFFFFFFF);
        context.eip = static_cast<uint32_t>(rip & 0xFFFFFFFF);
        context.eflags = static_cast<uint32_t>(rflags & 0xFFFFFFFF);
        std::cout << "x64 Context saved for VM " << vmId << std::endl;
    }
    
    void loadContext() override {
        // 从32位context结构体恢复64位寄存器状态（零扩展）
        rax = context.eax;
        rbx = context.ebx;
        rcx = context.ecx;
        rdx = context.edx;
        rsi = context.esi;
        rdi = context.edi;
        rbp = context.ebp;
        rsp = context.esp;
        rip = context.eip;
        rflags = context.eflags;
        std::cout << "x64 Context loaded for VM " << vmId << std::endl;
    }
    
    bool runOneInstruction() override {
        if (!isRunning || !payload || instructionCount >= resourceLimit) {
            return false;
        }
        
        // 简单的指令模拟
        if (rip >= payloadSize) {
            stop();
            return false;
        }
        
        // 获取指令字节
        uint8_t opcode = payload[rip];
        executeX64Instruction(opcode);
        
        rip++;
        instructionCount++;
        
        // 检查是否达到资源限制
        if (instructionCount >= resourceLimit) {
            std::cout << "x64 VM " << vmId << " reached resource limit" << std::endl;
            pause();
            return false;
        }
        
        return true;
    }
    
    bool runOneSlice() override {
        // 执行固定数量的指令作为时间片
        const int SLICE_INSTRUCTIONS = 10;
        int executed = 0;
        
        for (int i = 0; i < SLICE_INSTRUCTIONS && isRunning; i++) {
            if (runOneInstruction()) {
                executed++;
            } else {
                break;
            }
        }
        
        std::cout << "x64 VM " << vmId << " executed " << executed << " instructions in slice" << std::endl;
        return executed > 0;
    }
    
    uint32_t getResourceUsage() override {
        return instructionCount;
    }
    
    void setResourceLimit(uint32_t limit) override {
        resourceLimit = limit;
        std::cout << "x64 VM " << vmId << " resource limit set to " << limit << std::endl;
    }
    
    // x64特有方法
    uint64_t getRegister64(const std::string& regName) const {
        if (regName == "rax") return rax;
        if (regName == "rbx") return rbx;
        if (regName == "rcx") return rcx;
        if (regName == "rdx") return rdx;
        if (regName == "rsi") return rsi;
        if (regName == "rdi") return rdi;
        if (regName == "rbp") return rbp;
        if (regName == "rsp") return rsp;
        if (regName == "rip") return rip;
        if (regName == "r8") return r8;
        if (regName == "r9") return r9;
        if (regName == "r10") return r10;
        if (regName == "r11") return r11;
        if (regName == "r12") return r12;
        if (regName == "r13") return r13;
        if (regName == "r14") return r14;
        if (regName == "r15") return r15;
        return 0;
    }
    
    void setRegister64(const std::string& regName, uint64_t value) {
        if (regName == "rax") rax = value;
        else if (regName == "rbx") rbx = value;
        else if (regName == "rcx") rcx = value;
        else if (regName == "rdx") rdx = value;
        else if (regName == "rsi") rsi = value;
        else if (regName == "rdi") rdi = value;
        else if (regName == "rbp") rbp = value;
        else if (regName == "rsp") rsp = value;
        else if (regName == "rip") rip = value;
        else if (regName == "r8") r8 = value;
        else if (regName == "r9") r9 = value;
        else if (regName == "r10") r10 = value;
        else if (regName == "r11") r11 = value;
        else if (regName == "r12") r12 = value;
        else if (regName == "r13") r13 = value;
        else if (regName == "r14") r14 = value;
        else if (regName == "r15") r15 = value;
    }
    
private:
    /**
     * @brief 执行x64指令
     * @param opcode 指令操作码
     */
    void executeX64Instruction(uint8_t opcode) {
        switch (opcode) {
            case 0x48: // REX前缀（64位操作）
                // 在这个简化实现中，我们假设后续指令是64位操作
                // 实际x64处理器会解析REX字节来确定操作数大小
                break;
                
            case 0x89: // MOV r/m64, r64 (简化版本)
                // 这里简化处理，实际需要解析ModR/M字节
                break;
                
            case 0x01: // ADD r/m64, r64 (简化版本)
                // 简化的加法操作示例
                rax += rbx;
                updateFlags64(rax);
                break;
                
            case 0x29: // SUB r/m64, r64 (简化版本)
                rax -= rbx;
                updateFlags64(rax);
                break;
                
            case 0xFF: // INC r64 (简化版本)
                rax++;
                updateFlags64(rax);
                break;
                
            case 0xFE: // DEC r64 (简化版本)
                rax--;
                updateFlags64(rax);
                break;
                
            case 0x50: // PUSH r64 (简化版本)
                push64(rax);
                break;
                
            case 0x58: // POP r64 (简化版本)
                rax = pop64();
                break;
                
            default:
                // 未知指令，简单跳过
                break;
        }
    }
    
    /**
     * @brief 64位压栈操作
     * @param value 要压入栈的值
     */
    void push64(uint64_t value) {
        rsp -= 8;  // 64位栈增长方向向下
        // 在实际实现中，这里应该将值写入内存
        // 简化处理：只更新栈指针
    }
    
    /**
     * @brief 64位出栈操作
     * @return 从栈顶弹出的值
     */
    uint64_t pop64() {
        uint64_t value = 0;  // 简化处理：返回0
        rsp += 8;  // 64位栈增长方向向下
        return value;
    }
    
    /**
     * @brief 更新64位标志位
     * @param result 运算结果
     */
    void updateFlags64(uint64_t result) {
        // 清除相关标志位
        rflags &= ~(FLAG_ZF | FLAG_SF | FLAG_OF);
        
        // 设置零标志
        if (result == 0) {
            rflags |= FLAG_ZF;
        }
        
        // 设置符号标志（检查最高位）
        if (result & 0x8000000000000000ULL) {
            rflags |= FLAG_SF;
        }
        
        // 溢出标志需要更复杂的检测逻辑，这里简化处理
    }
};

#endif // X64_VM_H