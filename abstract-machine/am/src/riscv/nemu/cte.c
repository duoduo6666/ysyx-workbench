#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};

#ifdef __riscv_e
    switch (c->gpr[15]) { // a5
#else
    switch (c->gpr[17]) { // a7
#endif
      case -1: ev.event = EVENT_YIELD; break;
      default: ev.event = EVENT_ERROR; break;
    }

    c->mepc += 4;
    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  asm volatile("csrw mstatus, %0" : : "r"(0x1800));

  // register event handler
  user_handler = handler;

  return true;
}

#if __riscv_xlen == 32
#define XLEN  4
#endif
#define CONTEXT_SIZE  ((NR_REGS + 3) * XLEN)

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context* context = kstack.end - 1 - CONTEXT_SIZE;
  memset(context, 0, CONTEXT_SIZE);
  context->gpr[2] = (uintptr_t)context - 1;
  context->gpr[10] = (uintptr_t)arg;
  context->mstatus = 0x1800;
  context->mepc = (uintptr_t)entry;
  return context;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
