VERILATOR = verilator
VERILATOR_CFLAGS += -cc --trace-fst --exe --build -Mdir build -O3 -Wall -j 1 --Wno-declfilename --Wno-unusedsignal # -MMD --x-assign fast --x-initial fast --noassert

BUILD_DIR = ./build

NVBOARD_FLAGS = $(NVBOARD_ARCHIVE) -$(addprefix -CFLAGS -I, $(NVBOARD_USR_INC)) $(addprefix -LDFLAGS , $(LDFLAGS))

CSRC = $(shell find csrc -name "*.cpp")
VSRC = $(shell find vsrc -name "*.v")

# READLINE_FLAGS = -LDFLAGS -lreadline
READLINE_FLAGS = $(addprefix -LDFLAGS , -lreadline)
LLVM_FLAGS = $(addprefix -CFLAGS , $(shell llvm-config --cxxflags))  $(addprefix -LDFLAGS , $(shell llvm-config --libs))

IMG = 

$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(BUILD_DIR)/labs)

all:
	$(VERILATOR) $(VERILATOR_CFLAGS) $(READLINE_FLAGS) $(LLVM_FLAGS) \
		--top-module ysyx_2070017_CPU -o cpu \
		$(CSRC) \
		vsrc/ysyx_2070017_defines.v $(VSRC)

sim: all
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	build/cpu -b $(IMG)

include ../Makefile
include $(NVBOARD_HOME)/scripts/nvboard.mk
