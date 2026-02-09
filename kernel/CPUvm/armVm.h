#ifndef ARM_VM_H
#define ARM_VM_H

#include "baseVM.h"
#include <iostream>
#include <stdexcept>

/**
 * @brief ARM VM实现类，模拟ARM架构的基本功能
 * @details 实现基本的ARM寄存器和常用指令集，支持大端/小端模式
 */
class ArmVm : public I_VmInterface {
private:
    // ARM标志位掩码
    static const uint32_t FLAG_N = 1 << 31;  // 负数标志
    static const uint32_t FLAG_Z = 1 << 30;  // 零标志
    static const uint32_t FLAG_C = 1 << 29;  // 进位标志
    static const uint32_t FLAG_V = 1 << 28;  // 溢出标志
    
    // ARM寄存器扩展
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7;  // 通用寄存器
    uint32_t r8, r9, r10, r11, r12;           // 通用寄存器
    uint32_t sp;                               // 栈指针寄存器
    uint32_t lr;                               // 链接寄存器
    uint32_t pc;                               // 程序计数器
    uint32_t cpsr;                             // 当前程序状态寄存器
    
    uint32_t resourceLimit;     // 资源限制
    uint32_t instructionCount;  // 已执行指令计数
    bool isBigEndian;           // 大端模式标志
    
public:
    /**
     * @brief 构造函数
     * @param id VM唯一标识符
     * @param bigEndian 是否使用大端模式，默认false（小端）
     */
    explicit ArmVm(uint32_t id, bool bigEndian = false) 
        : I_VmInterface(id), 
          r0(0), r1(0), r2(0), r3(0), r4(0), r5(0), r6(0), r7(0),
          r8(0), r9(0), r10(0), r11(0), r12(0), sp(0), lr(0), pc(0), cpsr(0),
          resourceLimit(10000), instructionCount(0), isBigEndian(bigEndian) {}
    
    // 实现基类的纯虚函数
    void start() override {
        if (isRunning) {
            throw std::runtime_error("VM already running");
        }
        isRunning = true;
        std::cout << "ARM VM " << vmId << " started" << (isBigEndian ? " (Big Endian)" : " (Little Endian)") << std::endl;
    }
    
    void pause() override {
        if (!isRunning) {
            throw std::runtime_error("VM not running");
        }
        isRunning = false;
        saveContext();
        std::cout << "ARM VM " << vmId << " paused" << std::endl;
    }
    
    void resume() override {
        if (isRunning) {
            throw std::runtime_error("VM already running");
        }
        loadContext();
        isRunning = true;
        std::cout << "ARM VM " << vmId << " resumed" << std::endl;
    }
    
    void stop() override {
        isRunning = false;
        std::cout << "ARM VM " << vmId << " stopped normally" << std::endl;
    }
    
    void forceStop() override {
        isRunning = false;
        std::cout << "ARM VM " << vmId << " force stopped" << std::endl;
    }
    
    void saveContext() override {
        // 将ARM寄存器状态保存到context结构体
        context.eax = r0;
        context.ebx = r1;
        context.ecx = r2;
        context.edx = r3;
        context.esi = r4;
        context.edi = r5;
        context.ebp = r11;  // ARM r11映射到ebp
        context.esp = sp;
        context.eip = pc;
        context.eflags = cpsr;
        std::cout << "ARM Context saved for VM " << vmId << std::endl;
    }
    
    void loadContext() override {
        // 从context结构体恢复ARM寄存器状态
        r0 = context.eax;
        r1 = context.ebx;
        r2 = context.ecx;
        r3 = context.edx;
        r4 = context.esi;
        r5 = context.edi;
        r11 = context.ebp;
        sp = context.esp;
        pc = context.eip;
        cpsr = context.eflags;
        std::cout << "ARM Context loaded for VM " << vmId << std::endl;
    }
    
    bool runOneInstruction() override {
        if (!isRunning || !payload || instructionCount >= resourceLimit) {
            return false;
        }
        
        // 简单的指令模拟
        if (pc >= payloadSize) {
            stop();
            return false;
        }
        
        // 获取指令字（考虑大小端模式）
        uint32_t instruction = readInstruction(pc);
        executeArmInstruction(instruction);
        
        pc += 4;  // ARM指令长度固定为4字节
        instructionCount++;
        
        // 检查是否达到资源限制
        if (instructionCount >= resourceLimit) {
            std::cout << "ARM VM " << vmId << " reached resource limit" << std::endl;
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
        
        std::cout << "ARM VM " << vmId << " executed " << executed << " instructions in slice" << std::endl;
        return executed > 0;
    }
    
    uint32_t getResourceUsage() override {
        return instructionCount;
    }
    
    void setResourceLimit(uint32_t limit) override {
        resourceLimit = limit;
        std::cout << "ARM VM " << vmId << " resource limit set to " << limit << std::endl;
    }
    
    // ARM特有方法
    void setEndianness(bool bigEndian) {
        isBigEndian = bigEndian;
        std::cout << "ARM VM " << vmId << " endianness set to " << (bigEndian ? "Big Endian" : "Little Endian") << std::endl;
    }
    
    bool getEndianness() const {
        return isBigEndian;
    }
    
private:
    /**
     * @brief 从payload读取ARM指令（考虑大小端）
     * @param address 内存地址
     * @return 32位指令字
     */
    uint32_t readInstruction(uint32_t address) {
        if (address + 3 >= payloadSize) {
            return 0;  // 内存越界，返回NOP指令
        }
        
        const uint8_t* ptr = payload + address;
        
        if (isBigEndian) {
            // 大端模式：高位字节在前
            return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        } else {
            // 小端模式：低位字节在前
            return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
        }
    }
    
    /**
     * @brief 执行ARM指令
     * @param instruction 32位ARM指令
     */
    void executeArmInstruction(uint32_t instruction) {
        // ARM指令格式分析
        uint32_t opcode = (instruction >> 21) & 0xF;  // 主操作码
        uint32_t rn = (instruction >> 16) & 0xF;      // 源寄存器
        uint32_t rd = (instruction >> 12) & 0xF;      // 目标寄存器
        uint32_t operand2 = instruction & 0xFFF;      // 操作数2
        
        switch (opcode) {
            case 0x0:  // AND
                setRegister(rd, getRegister(rn) & operand2);
                updateCpsr(getRegister(rd));
                break;
                
            case 0x1:  // EOR (XOR)
                setRegister(rd, getRegister(rn) ^ operand2);
                updateCpsr(getRegister(rd));
                break;
                
            case 0x2:  // SUB
                setRegister(rd, getRegister(rn) - operand2);
                updateCpsr(getRegister(rd));
                break;
                
            case 0x4:  // ADD
                setRegister(rd, getRegister(rn) + operand2);
                updateCpsr(getRegister(rd));
                break;
                
            case 0x5:  // ADC (带进位加法)
                setRegister(rd, getRegister(rn) + operand2 + ((cpsr & FLAG_C) ? 1 : 0));
                updateCpsr(getRegister(rd));
                break;
                
            case 0xD:  // MOV
                setRegister(rd, operand2);
                updateCpsr(getRegister(rd));
                break;
                
            case 0xE:  // B (分支)
                pc += (static_cast<int32_t>(operand2 << 8) >> 8) * 4;  // 符号扩展并转换为字节偏移
                break;
                
            default:
                // 未知指令，简单忽略
                break;
        }
    }
    
    /**
     * @brief 获取寄存器值
     * @param regNum 寄存器编号(0-15)
     * @return 寄存器值
     */
    uint32_t getRegister(uint32_t regNum) const {
        switch (regNum) {
            case 0: return r0;
            case 1: return r1;
            case 2: return r2;
            case 3: return r3;
            case 4: return r4;
            case 5: return r5;
            case 6: return r6;
            case 7: return r7;
            case 8: return r8;
            case 9: return r9;
            case 10: return r10;
            case 11: return r11;
            case 12: return r12;
            case 13: return sp;
            case 14: return lr;
            case 15: return pc;
            default: return 0;
        }
    }
    
    /**
     * @brief 设置寄存器值
     * @param regNum 寄存器编号(0-15)
     * @param value 要设置的值
     */
    void setRegister(uint32_t regNum, uint32_t value) {
        switch (regNum) {
            case 0: r0 = value; break;
            case 1: r1 = value; break;
            case 2: r2 = value; break;
            case 3: r3 = value; break;
            case 4: r4 = value; break;
            case 5: r5 = value; break;
            case 6: r6 = value; break;
            case 7: r7 = value; break;
            case 8: r8 = value; break;
            case 9: r9 = value; break;
            case 10: r10 = value; break;
            case 11: r11 = value; break;
            case 12: r12 = value; break;
            case 13: sp = value; break;
            case 14: lr = value; break;
            case 15: pc = value; break;
        }
    }
    
    /**
     * @brief 更新CPSR标志位
     * @param result 运算结果
     */
    void updateCpsr(uint32_t result) {
        // 清除NZCV标志位
        cpsr &= ~(FLAG_N | FLAG_Z | FLAG_C | FLAG_V);
        
        // 设置负数标志(N)
        if (result & 0x80000000) {
            cpsr |= FLAG_N;
        }
        
        // 设置零标志(Z)
        if (result == 0) {
            cpsr |= FLAG_Z;
        }
        
        // 进位标志(C)和溢出标志(V)需要根据具体运算设置
        // 这里简化处理
    }
};

#endif // ARM_VM_H