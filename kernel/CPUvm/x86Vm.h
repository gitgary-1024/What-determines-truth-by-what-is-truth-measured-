#ifndef X86_VM_H
#define X86_VM_H

#include "baseVM.h"
#include <iostream>
#include <stdexcept>

/**
 * @brief x86 VM实现类，模拟x86架构的基本功能
 * @details 实现基本的x86寄存器和常用指令集
 */
class X86Vm : public I_VmInterface {
private:
    // 标志位掩码
    static const uint32_t FLAG_CF = 1 << 0;  // 进位标志
    static const uint32_t FLAG_PF = 1 << 2;  // 奇偶标志
    static const uint32_t FLAG_AF = 1 << 4;  // 辅助进位标志
    static const uint32_t FLAG_ZF = 1 << 6;  // 零标志
    static const uint32_t FLAG_SF = 1 << 7;  // 符号标志
    static const uint32_t FLAG_OF = 1 << 11; // 溢出标志
    
    uint32_t resourceLimit;     // 资源限制
    uint32_t instructionCount;  // 已执行指令计数
    
public:
    /**
     * @brief 构造函数
     * @param id VM唯一标识符
     */
    explicit X86Vm(uint32_t id) : I_VmInterface(id), 
                                 resourceLimit(10000), 
                                 instructionCount(0) {}
    
    // 实现基类的纯虚函数
    void start() override {
        if (isRunning) {
            throw std::runtime_error("VM already running");
        }
        isRunning = true;
        std::cout << "VM " << vmId << " started" << std::endl;
    }
    
    void pause() override {
        if (!isRunning) {
            throw std::runtime_error("VM not running");
        }
        isRunning = false;
        saveContext();
        std::cout << "VM " << vmId << " paused" << std::endl;
    }
    
    void resume() override {
        if (isRunning) {
            throw std::runtime_error("VM already running");
        }
        loadContext();
        isRunning = true;
        std::cout << "VM " << vmId << " resumed" << std::endl;
    }
    
    void stop() override {
        isRunning = false;
        std::cout << "VM " << vmId << " stopped normally" << std::endl;
    }
    
    void forceStop() override {
        isRunning = false;
        std::cout << "VM " << vmId << " force stopped" << std::endl;
    }
    
    void saveContext() override {
        // 上下文已经在context成员变量中维护
        std::cout << "Context saved for VM " << vmId << std::endl;
    }
    
    void loadContext() override {
        // 从保存的上下文恢复
        std::cout << "Context loaded for VM " << vmId << std::endl;
    }
    
    bool runOneInstruction() override {
        if (!isRunning || !payload || instructionCount >= resourceLimit) {
            return false;
        }
        
        // 简单的指令模拟 - 实际项目中会更复杂
        if (context.eip >= payloadSize) {
            stop();
            return false;
        }
        
        // 模拟执行一条指令（这里简化处理）
        uint8_t opcode = payload[context.eip];
        executeInstruction(opcode);
        
        context.eip++;
        instructionCount++;
        
        // 检查是否达到资源限制
        if (instructionCount >= resourceLimit) {
            std::cout << "VM " << vmId << " reached resource limit" << std::endl;
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
        
        std::cout << "VM " << vmId << " executed " << executed << " instructions in slice" << std::endl;
        return executed > 0;
    }
    
    uint32_t getResourceUsage() override {
        return instructionCount;
    }
    
    void setResourceLimit(uint32_t limit) override {
        resourceLimit = limit;
        std::cout << "VM " << vmId << " resource limit set to " << limit << std::endl;
    }
    
private:
    /**
     * @brief 执行单条指令
     * @param opcode 指令操作码
     */
    void executeInstruction(uint8_t opcode) {
        switch (opcode) {
            case 0x00: // NOP
                break;
                
            case 0x01: // MOV EAX, EBX
                context.eax = context.ebx;
                break;
                
            case 0x02: // ADD EAX, EBX
                context.eax += context.ebx;
                updateFlags(context.eax);
                break;
                
            case 0x03: // SUB EAX, EBX
                context.eax -= context.ebx;
                updateFlags(context.eax);
                break;
                
            case 0x04: // INC EAX
                context.eax++;
                updateFlags(context.eax);
                break;
                
            case 0x05: // DEC EAX
                context.eax--;
                updateFlags(context.eax);
                break;
                
            case 0x06: // PUSH EAX
                if (context.esp >= 4 && context.esp <= context.stack.size() * sizeof(uint32_t)) {
                    context.stack[(context.esp - 4) / sizeof(uint32_t)] = context.eax;
                    context.esp -= 4;
                }
                break;
                
            case 0x07: // POP EAX
                if (context.esp < context.stack.size() * sizeof(uint32_t)) {
                    context.eax = context.stack[context.esp / sizeof(uint32_t)];
                    context.esp += 4;
                }
                break;
                
            default:
                // 未知指令，简单跳过
                break;
        }
    }
    
    /**
     * @brief 更新标志位
     * @param result 运算结果
     */
    void updateFlags(uint32_t result) {
        // 清除相关标志位
        context.eflags &= ~(FLAG_ZF | FLAG_SF);
        
        // 设置零标志
        if (result == 0) {
            context.eflags |= FLAG_ZF;
        }
        
        // 设置符号标志（检查最高位）
        if (result & 0x80000000) {
            context.eflags |= FLAG_SF;
        }
    }
};

#endif // X86_VM_H