#include "device/mmio.h"
#include "memory/mmu.h"
#include "nemu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type)                                                    \
  *(type *)({                                                                  \
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound",       \
           addr);                                                              \
    guest_to_host(addr);                                                       \
  })

// »ñÈ¡pte
#define PTE_ADDR(pte) ((uint32_t)(pte) & ~0xfff)
// ½âÎöva
#define PDX(va) (((uint32_t)(va) >> 22) & 0x3ff)
#define PTX(va) (((uint32_t)(va) >> 12) & 0x3ff)
#define OFF(va) ((uint32_t)(va)&0xfff)

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  if (is_mmio(addr) != -1)
    return mmio_read(addr, len, is_mmio(addr));
  else
    return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  if (is_mmio(addr) != -1)
    mmio_write(addr, len, data, is_mmio(addr));
  else
    memcpy(guest_to_host(addr), &data, len);
}

paddr_t page_translate(vaddr_t addr, bool worr) {
  CR0 cr0 = (CR0)cpu.CR0;
  if (cr0.paging && cr0.protect_enable) {
    CR3 cr3 = (CR3)cpu.CR3;
    PDE *pgdir = (PDE *)PTE_ADDR(cr3.val);
    PDE pde = (PDE)paddr_read((uint32_t)(pgdir + PDX(addr)), 4);
    Assert(pde.present, "addr=0x%x", addr);
    pde.accessed = 1;

    PTE *ptab = (PTE *)PTE_ADDR(pde.val);
    PTE pte = (PTE)paddr_read((uint32_t)(ptab + PTX(addr)), 4);
    Assert(pte.present, "addr=0x%x", addr);
    pte.accessed = 1;
    pte.dirty = worr ? 1 : pte.dirty;
    return PTE_ADDR(pte.val) | OFF(addr);
  }
  return addr;
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if (PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)) {
    uint32_t data = 0;
    for (int i = 0; i < len; i++) {
      paddr_t paddr = page_translate(addr + i, false);
      data += (paddr_read(paddr, 1)) << 8 * i;
    }
    return data;
  } else {
    paddr_t paddr = page_translate(addr, false);
    return paddr_read(paddr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if (PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)) {
    for (int i = 0; i < len; i++) {
      paddr_t paddr = page_translate(addr + i, true);
      paddr_write(paddr, 1, data >> 8 * i);
    }
    return;
  } else {
    paddr_t paddr = page_translate(addr, true);
    paddr_write(paddr, len, data);
    return;
  }
}
