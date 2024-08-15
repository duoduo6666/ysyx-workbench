VERILATOR = verilator
VERILATOR_CFLAGS += -cc --trace-fst --exe --build -Mdir build -O3 -Wall -j 1 --Wno-declfilename --Wno-unusedsignal # -MMD --x-assign fast --x-initial fast --noassert

BUILD_DIR = ./build

include $(NVBOARD_HOME)/scripts/nvboard.mk

NVBOARD_FLAGS = $(NVBOARD_ARCHIVE) -$(addprefix -CFLAGS -I, $(NVBOARD_USR_INC)) $(addprefix -LDFLAGS , $(LDFLAGS))

$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(BUILD_DIR)/labs)

all:
	@echo "Write this Makefile by your self."

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module ysyx_2070017_CPU -o cpu \
	        csrc/main.cpp vsrc/defines.v vsrc/CPU.v vsrc/IDU.v vsrc/ALU.v vsrc/RF.v vsrc/Mux.v vsrc/Reg.v
	build/cpu +trace

include ../Makefile
