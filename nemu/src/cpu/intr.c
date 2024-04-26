#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  // 触发中断/异常，NO 是中断号，ret_addr 是返回地址

  // 压入状态
  memcpy(&t1, &cpu.eflags, sizeof(cpu.eflags)); // 保存 eflags
  rtl_li(&t0, t1);
  rtl_push(&t0);
  rtl_push(&cpu.cs);
  rtl_li(&t0, ret_addr);
  rtl_push(&t0);

  vaddr_t gate_addr =
      cpu.idtr.base + NO * sizeof(GateDesc); // 计算 IDT 条目地址
  assert(gate_addr <= cpu.idtr.base + cpu.idtr.limit);

  uint32_t off_15_0 = vaddr_read(gate_addr, 2); // 读取偏移低 16 位
  uint32_t off_32_16 =
      vaddr_read(gate_addr + sizeof(GateDesc) - 2, 2); // 读取偏移高 16 位

  uint32_t target_addr = (off_32_16 << 16) + off_15_0; // 计算目标地址
#ifdef DEBUG
  Log("target_addr=0x%x", target_addr);
#endif
  decoding.is_jmp = 1;            // 设置跳转标志
  decoding.jmp_eip = target_addr; // 设置跳转地址
}

void dev_raise_intr() {}
