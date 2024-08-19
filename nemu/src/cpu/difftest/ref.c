/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <difftest-def.h>
#include <memory/paddr.h>
#include "../local-include/reg.h"

__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  if (direction) {
    for (size_t i = 0; i < n; i++){
      paddr_write(addr + i, 1, ((uint8_t*)buf)[i]);
    }
  } else {
    for (size_t i = 0; i < n; i++){
      ((uint8_t*)buf)[i] = paddr_read(addr + i, 1);
    }
  }
}

__EXPORT void difftest_regcpy(void *dut, bool direction) {
  if (direction) {
    for (size_t i = 0; i < RISCV_GPR_NUM; i++){
      gpr(i) = ((word_t*)dut)[i];
    }
  } else {
    for (size_t i = 0; i < RISCV_GPR_NUM; i++){
      ((word_t*)dut)[i] = gpr(i);
    }
  }
}

__EXPORT void difftest_pccpy(void *dut, bool direction) {
  if (direction) {
    cpu.pc = *(word_t*)dut;
  } else {
    *(word_t*)dut = cpu.pc;
  }
}

__EXPORT void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

__EXPORT void difftest_raise_intr(word_t NO) {
  assert(0);
}

__EXPORT void difftest_init(int port) {
  void init_mem();
  init_mem();
  /* Perform ISA dependent initialization. */
  init_isa();
}
