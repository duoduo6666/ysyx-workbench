VERILATOR = verilator
VERILATOR_CFLAGS += -cc --trace-fst --exe --build -Mdir build -O3 -Wall -j 1 # -MMD --x-assign fast --x-initial fast --noassert

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

light:
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py constr/light.nxdc $(BUILD_DIR)/light_auto_bind.cpp
	$(VERILATOR) $(VERILATOR_CFLAGS) $(NVBOARD_FLAGS) \
		--top-module light -o light \
	        $(BUILD_DIR)/light_auto_bind.cpp csrc/light.cpp vsrc/light.v
	$(BUILD_DIR)/light 

DoubleControlSwitch:
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py constr/DoubleControlSwitch.nxdc $(BUILD_DIR)/DoubleControlSwitch_auto_bind.cpp
	$(VERILATOR) $(VERILATOR_CFLAGS) $(NVBOARD_FLAGS)\
		--top-module DoubleControlSwitch -o DoubleControlSwitch \
		$(BUILD_DIR)/DoubleControlSwitch_auto_bind.cpp csrc/DoubleControlSwitch.cpp vsrc/DoubleControlSwitch.v
	$(BUILD_DIR)/DoubleControlSwitch

lab1:
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py constr/labs/lab1.nxdc $(BUILD_DIR)/labs/lab1_auto_bind.cpp
	$(VERILATOR) $(VERILATOR_CFLAGS) $(NVBOARD_FLAGS) \
		--top-module lab1 -o labs/lab1 \
	        $(BUILD_DIR)/labs/lab1_auto_bind.cpp csrc/labs/lab1.cpp vsrc/labs/lab1.v
	$(BUILD_DIR)/labs/lab1

lab2:
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py constr/labs/lab2.nxdc $(BUILD_DIR)/labs/lab2_auto_bind.cpp
	$(VERILATOR) $(VERILATOR_CFLAGS) $(NVBOARD_FLAGS) \
		--top-module lab2 -o labs/lab2 \
	        $(BUILD_DIR)/labs/lab2_auto_bind.cpp csrc/labs/lab2.cpp vsrc/labs/lab2.v
	$(BUILD_DIR)/labs/lab2

include ../Makefile
