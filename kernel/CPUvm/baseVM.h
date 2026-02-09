#ifndef BASE_VM_H
#define BASE_VM_H

#include <cstdint>
#include <memory>
#include <vector>

/**
 * @brief VM上下文结构体，保存寄存器状态和标志位
 * @details 包含x86架构的基本寄存器和标志位状态
 */
struct S_VmContext {
    uint32_t eax;      // 累加器寄存器
    uint32_t ebx;      // 基址寄存器
    uint32_t ecx;      // 计数寄存器
    uint32_t edx;      // 数据寄存器
    uint32_t esi;      // 源变址寄存器
    uint32_t edi;      // 目的变址寄存器
    uint32_t ebp;      // 基址指针寄存器
    uint32_t esp;      // 栈指针寄存器
    uint32_t eip;      // 指令指针寄存器
    uint32_t eflags;   // 标志寄存器
    
    // 栈空间
    std::vector<uint32_t> stack;
    
    S_VmContext() : eax(0), ebx(0), ecx(0), edx(0), 
                   esi(0), edi(0), ebp(0), esp(0), 
                   eip(0), eflags(0) {
        stack.resize(1024, 0); // 初始化1KB栈空间
    }
};

/**
 * @brief VM统一接口类，为x86/arm/x64 VM提供统一的操作接口
 * @details 所有具体的VM实现都需要继承此类并实现相应的方法
 */
class I_VmInterface {
protected:
    uint32_t vmId;              // VM唯一标识符
    S_VmContext context;        // VM上下文状态
    bool isRunning;             // VM运行状态
    const uint8_t* payload;     // 指令载荷指针
    size_t payloadSize;         // 载荷大小
    
public:
    /**
     * @brief 构造函数
     * @param id VM唯一标识符
     */
    explicit I_VmInterface(uint32_t id) 
        : vmId(id), isRunning(false), payload(nullptr), payloadSize(0) {}
    
    virtual ~I_VmInterface() = default;
    
    // 基本控制方法
    virtual void start() = 0;           // 启动VM开始执行指令
    virtual void pause() = 0;           // 暂停VM保存当前状态
    virtual void resume() = 0;          // 恢复VM继续执行
    virtual void stop() = 0;            // 正常停止VM释放资源
    virtual void forceStop() = 0;       // 强制终止VM（管理员端专用）
    
    // 上下文管理方法
    virtual void saveContext() = 0;     // 保存VM上下文状态
    virtual void loadContext() = 0;     // 恢复VM上下文状态
    
    // 指令执行方法
    virtual bool runOneInstruction() = 0;   // 执行一条指令
    virtual bool runOneSlice() = 0;         // 执行一个时间片
    
    // 资源管理方法
    virtual uint32_t getResourceUsage() = 0;    // 获取VM资源使用情况
    virtual void setResourceLimit(uint32_t limit) = 0; // 设置VM资源限制
    
    // Getter方法
    uint32_t getVmId() const { return vmId; }
    bool getRunningStatus() const { return isRunning; }
    const S_VmContext& getContext() const { return context; }
    
    // 载荷设置方法（为第二阶段预留接口）
    virtual void setPayload(const uint8_t* data, size_t size) {
        payload = data;
        payloadSize = size;
    }
    
    virtual const uint8_t* getPayload() const { return payload; }
    virtual size_t getPayloadSize() const { return payloadSize; }
};

#endif // BASE_VM_H