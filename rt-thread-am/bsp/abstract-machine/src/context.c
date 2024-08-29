#include <am.h>
#include <klib.h>
#include <rtthread.h>

static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD: 
      if ((c->gpr[11]) != 0) *(Context**)(c->gpr[11]) = c;
      return *(Context**)(c->gpr[10]);
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  asm volatile("mv a0, %0" : : "r"(to) : "a0");
  asm volatile("mv a1, %0" : : "r"(0) : "a1");
  yield();
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  asm volatile("mv a0, %0" : : "r"(to) : "a0");
  asm volatile("mv a1, %0" : : "r"(from) : "a1");
  yield();
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

void wrap_func(void (*tentry)(void *), void *parameter, void (*texit)()) {
  tentry(parameter);
  texit();
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  stack_addr -= (uintptr_t)stack_addr % sizeof(uintptr_t);
  assert((uintptr_t)stack_addr % sizeof(uintptr_t) == 0);

  Context *context = kcontext((Area){stack_addr, stack_addr}, (void (*)(void (*)))wrap_func, tentry);
  context->gpr[11] = (uintptr_t)parameter;
  context->gpr[12] = (uintptr_t)texit;
  printf("init %p\n", context);
  return (rt_uint8_t*)context;
}
