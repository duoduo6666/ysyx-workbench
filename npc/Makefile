all:
	@echo "Write this Makefile by your self."

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."
	verilator --trace -cc --exe --build -j 1 -Wall csrc/DoubleControlSwitch.cpp vsrc/DoubleControlSwitch.v
	obj_dir/VDoubleControlSwitch +trace


include ../Makefile
include $(NVBOARD_HOME)/scripts/nvboard.mk
