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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include <elf.h>

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

void device_update();

bool check_wp();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
  if (check_wp()) {nemu_state.state = NEMU_STOP;}
}


// iringbuf
static word_t iringbuf[MAX_INST_TO_PRINT] = {0};
static int iringbuf_end = 0;

void itrace(char *buf, size_t buf_len, word_t pc, uint32_t inst_val, int inst_len) {
  char *p = buf;
  p += snprintf(p, buf_len, FMT_WORD ":", pc);
  int ilen = inst_len;
  int i;
  uint8_t *inst = (uint8_t *)&inst_val;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

  #ifndef CONFIG_ISA_loongarch32r
    void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
    disassemble(p, buf + buf_len - p,
        MUXDEF(CONFIG_ISA_x86, s->snpc, pc), (uint8_t *)&inst_val, ilen);
  #else
    p[0] = '\0'; // the upstream llvm does not support loongarch32r
  #endif
}


Elf32_Sym* symbols;
int symbol_num;
char* strtab;
int ftrace_deep = 0;
#ifdef CONFIG_FTRACE
char* ftrace_stack[CONFIG_FTRACE_STACK_SIZE] = {0};
#endif
void init_ftrace(char* elf_file) {
  
  assert(elf_file != NULL);

  FILE* fp = fopen(elf_file, "r");
  assert(fp != NULL);

  Elf32_Ehdr elf;
  assert(fread(&elf, sizeof(Elf32_Ehdr), 1, fp) == 1);

  assert(fseek(fp, elf.e_shoff, SEEK_SET) == 0);
  Elf32_Shdr* shdr = malloc(elf.e_shnum * sizeof(Elf32_Shdr));
  assert(shdr != NULL);
  assert(fread(shdr, sizeof(Elf32_Shdr), elf.e_shnum, fp) == elf.e_shnum);

  char* shstrtab = NULL;
  shstrtab = malloc(shdr[elf.e_shstrndx].sh_size);
  assert(shstrtab != NULL);
  assert(fseek(fp, shdr[elf.e_shstrndx].sh_offset, SEEK_SET) == 0);
  assert(fread(shstrtab, sizeof(char), shdr[elf.e_shstrndx].sh_size, fp) == shdr[elf.e_shstrndx].sh_size);

  Elf32_Shdr* symtab_hdr = NULL;
  strtab = NULL;
  for (int i = 0; i < elf.e_shnum; i++) {
    // Log("[%d] Name: %s Type: %x", i, shstrtab + shdr[i].sh_name, shdr[i].sh_type);
    if (shdr[i].sh_type == SHT_SYMTAB) {
      symtab_hdr = &shdr[i];
    }
    if (strcmp(shstrtab + shdr[i].sh_name, ".strtab") == 0) {
      strtab = malloc(shdr[i].sh_size);
      assert(fseek(fp, shdr[i].sh_offset, SEEK_SET) == 0);
      assert(fread(strtab, sizeof(char), shdr[i].sh_size, fp) == shdr[i].sh_size);
    }
  }
  assert(symtab_hdr != NULL);

  symbol_num = symtab_hdr->sh_size / sizeof(Elf32_Sym);
  symbols = malloc(symtab_hdr->sh_size);
  assert(symbols != NULL);
  assert(fseek(fp, symtab_hdr->sh_offset, SEEK_SET) == 0);
  assert(fread(symbols, sizeof(char), symtab_hdr->sh_size, fp) == symtab_hdr->sh_size);
  for (int i = 0; i < symbol_num; i++) {
    if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC) {
      Log("[%d] Name %s Value 0x%x Size 0x%x", i, strtab + symbols[i].st_name, symbols[i].st_value, symbols[i].st_size);
    }
  }

  fclose(fp);
}

int ftrace_pc_in_func(vaddr_t pc) {
  for (int i = 0; i < symbol_num; i++) {
    if (pc > symbols[i].st_value && pc < symbols[i].st_value + symbols[i].st_size){
      return i;
    }
  }
  return -1;
}

void ftrace_print_deep() {
  for (int i = 0; i < ftrace_deep; i++) {
    printf("  ");
  }
}

int ftrace_search_stack(char* func) {
  for (int i = ftrace_deep - 1; i >= 0; i--) {
    if (strcmp(ftrace_stack[i], func)) {
      return i + 1;
    }
  }
  return -1;
}

void ftrace_check(vaddr_t pc, vaddr_t next_pc) {
  for (int i = 0; i < symbol_num; i++) {
    if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC) {
      if (next_pc == symbols[i].st_value) {
        printf("0x%x: ", pc);
        ftrace_print_deep();
        printf("cell[%s@0x%x]\n", strtab + symbols[i].st_name, symbols[i].st_value);
        ftrace_stack[ftrace_deep] = strtab + symbols[i].st_name;
        ftrace_deep++;
        assert(ftrace_deep < CONFIG_FTRACE_STACK_SIZE);
        return;
      }
    }
  }
  for (int i = 0; i < symbol_num; i++) {
    if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC) {
      if (ftrace_pc_in_func(pc) == i && (next_pc > symbols[i].st_value + symbols[i].st_size || next_pc < symbols[i].st_value)) {
        ftrace_deep = ftrace_search_stack(strtab + symbols[i].st_name);
        assert(ftrace_deep > 0);
        printf("0x%x: ", pc);
        ftrace_print_deep();
        printf("ret[%s@0x%x]\n", strtab + symbols[i].st_name, next_pc);
        break;
      }
    }
  }
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  itrace(s->logbuf, sizeof(s->logbuf), pc, s->isa.inst.val, s->snpc - s->pc);

  // iringbuf
  iringbuf[iringbuf_end] = pc;
  iringbuf_end = (iringbuf_end+1) % MAX_INST_TO_PRINT;
#endif
}

static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

word_t vaddr_ifetch(vaddr_t addr, int len);
void iringbuf_inst_output(word_t pc, bool is_end) {
  char buf[128];
  itrace(buf+3, sizeof(buf)-3, pc, vaddr_ifetch(pc, 4), 4);
  if (is_end) {
    buf[0] = '-'; buf[1] = '-'; buf[2] = '>';
  } else {
    buf[0] = ' '; buf[1] = ' '; buf[2] = ' ';
  }
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", buf); }
#endif
  puts(buf);
}

void iringbuf_display() {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("iringbuf"); }
#endif
  puts("iringbuf: ");
  if (iringbuf[(iringbuf_end+1) % MAX_INST_TO_PRINT] == 0){
    for (int i = 0; i < iringbuf_end-1; i++) {
      iringbuf_inst_output(iringbuf[i], false);
    }
    iringbuf_inst_output(iringbuf[iringbuf_end-1], true);
  } else {
    for (int i = iringbuf_end; i < MAX_INST_TO_PRINT; i++) {
      iringbuf_inst_output(iringbuf[i], false);
    }
    for (int i = 0; i < iringbuf_end-1; i++) {
      iringbuf_inst_output(iringbuf[i], false);
    }
    iringbuf_inst_output(iringbuf[iringbuf_end-1], true);
  }
}

void assert_fail_msg() {
  isa_reg_display();
#ifdef CONFIG_ITRACE
#ifdef CONFIG_ISA_riscv
  iringbuf_display();
#endif
#endif
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT: statistic();
  }
}
