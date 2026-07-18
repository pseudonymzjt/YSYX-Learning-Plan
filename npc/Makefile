V = verilator
VFLAGS = -cc --exe --build --trace-fst -Mdir build -y vsrc

# 默认值
TARGET ?= test.v
SRC ?= test.cpp
DIR = build
LIB = V$(basename $(notdir $(TARGET)))  # 自动从TARGET提取LIB名

all:
	@echo "Write this Makefile by your self."

sim: 
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Compiling $(TARGET) with $(SRC)"
	$(V) $(VFLAGS) $(TARGET) $(SRC)
	$(DIR)/$(LIB)

include ../Makefile
