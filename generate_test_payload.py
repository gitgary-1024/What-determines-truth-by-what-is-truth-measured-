#!/usr/bin/env python3
"""
测试payload文件生成脚本
为MyOS VM系统生成各种架构的测试二进制文件
"""

import struct
import os

def generate_x86_payload():
    """生成x86架构测试payload"""
    print("Generating x86 test payload...")
    
    # x86指令序列
    payload = bytearray()
    
    # mov eax, 1
    payload.extend([0xB8, 0x01, 0x00, 0x00, 0x00])
    
    # add eax, 100 (0x64)
    payload.extend([0x05, 0x64, 0x00, 0x00, 0x00])
    
    # inc eax
    payload.append(0x40)
    
    # dec eax  
    payload.append(0x48)
    
    # push eax
    payload.append(0x50)
    
    # pop eax
    payload.append(0x58)
    
    # nop
    payload.append(0x90)
    
    # 循环跳转 (-12 bytes backwards)
    payload.extend([0xEB, 0xF0])
    
    # 保存文件
    with open('x86_test.bin', 'wb') as f:
        f.write(payload)
    
    print(f"x86 payload generated ({len(payload)} bytes)")

def generate_arm_payload():
    """生成ARM架构测试payload"""
    print("Generating ARM test payload...")
    
    # ARM指令序列 (小端模式)
    payload = bytearray()
    
    # mov r0, #1
    payload.extend([0x01, 0x00, 0xA0, 0xE3])
    
    # add r0, r0, #100
    payload.extend([0x64, 0x00, 0x80, 0xE2])
    
    # cmp r0, #50
    payload.extend([0x32, 0x00, 0x50, 0xE3])
    
    # bne -12 (跳转到开始，形成循环)
    payload.extend([0xF9, 0xFF, 0xFF, 0x1A])
    
    # 保存文件
    with open('arm_test.bin', 'wb') as f:
        f.write(payload)
    
    print(f"ARM payload generated ({len(payload)} bytes)")

def generate_x64_payload():
    """生成x64架构测试payload"""
    print("Generating x64 test payload...")
    
    # x64指令序列
    payload = bytearray()
    
    # mov rax, 1
    payload.extend([0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00])
    
    # add rax, 100
    payload.extend([0x48, 0x05, 0x64, 0x00, 0x00, 0x00])
    
    # inc rax
    payload.extend([0x48, 0xFF, 0xC0])
    
    # dec rax
    payload.extend([0x48, 0xFF, 0xC8])
    
    # push rax
    payload.append(0x50)
    
    # pop rax
    payload.append(0x58)
    
    # jmp -19 (跳转到开始)
    payload.extend([0xEB, 0xE9])
    
    # 保存文件
    with open('x64_test.bin', 'wb') as f:
        f.write(payload)
    
    print(f"x64 payload generated ({len(payload)} bytes)")

def generate_simple_payload():
    """生成简单测试payload用于基本测试"""
    print("Generating simple test payload...")
    
    # 简单的字节序列
    payload = bytearray([0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08])
    
    with open('test_payload.bin', 'wb') as f:
        f.write(payload)
    
    print(f"Simple payload generated ({len(payload)} bytes)")

def main():
    print("=" * 50)
    print("MyOS VM System - Test Payload Generator")
    print("=" * 50)
    
    # 创建payload文件
    generate_x86_payload()
    generate_arm_payload()
    generate_x64_payload()
    generate_simple_payload()
    
    print("\n" + "=" * 50)
    print("All test payloads generated successfully!")
    print("=" * 50)
    print("Generated files:")
    print("- x86_test.bin  (x86 architecture)")
    print("- arm_test.bin  (ARM architecture)")
    print("- x64_test.bin  (x64 architecture)")
    print("- test_payload.bin (simple test)")
    print("=" * 50)

if __name__ == "__main__":
    main()