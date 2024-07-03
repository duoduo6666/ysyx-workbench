VERILATOR = verilator
VERILATOR_CFLAGS += -cc --trace-fst --exe --build -Mdir build -O3 -Wall -j 1 # -MMD --x-assign fast --x-initial fast --noassert

BUILD_DIR = ./build

include $(NVBOARD_HOME)/scripts/nvboard.mk

NVBOARD_FLAGS = $(NVBOARD_ARCHIVE) -$(addprefix -CFLAGS -I, $(NVBOARD_USR_INC)) $(addprefix -LDFLAGS , $(LDFLAGS))

$(shell mkdir -p $(BUILD_DIR))

all:
	@echo "Write this Makefile by your self."

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py constr/light.nxdc $(BUILD_DIR)/light_auto_bind.cpp
	$(VERILATOR) $(VERILATOR_CFLAGS) $(NVBOARD_FLAGS) \
		--top-module light -o light \
	        build/light_auto_bind.cpp csrc/light.cpp vsrc/light.v
	$(BUILD_DIR)/light 

DoubleControlSwitch:
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module DoubleControlSwitch -o DoubleControlSwitch \
		csrc/DoubleControlSwitch.cpp vsrc/DoubleControlSwitch.v
	obj_dir/VDoubleControlSwitch +trace

include ../Makefile
