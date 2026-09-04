TARGET    := packet-analyzer
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

CXX      ?= g++
CXXSTD   := -std=c++17
WARNINGS := -Wall -Wextra -Wpedantic
CPPFLAGS += -I$(SRC_DIR) -MMD -MP
CXXFLAGS += $(CXXSTD) $(WARNINGS)
LDLIBS   += -lpcap

BUILD ?= release
ifeq ($(BUILD),debug)
    CXXFLAGS += -Og -g3
else ifeq ($(BUILD),release)
    CXXFLAGS += -O2 -DNDEBUG
else
$(error unknown BUILD "$(BUILD)" -- use debug or release)
endif

ifdef SANITIZE
SAN      := -fsanitize=address,undefined -fno-omit-frame-pointer
CXXFLAGS += $(SAN)
LDFLAGS  += $(SAN)
endif

OBJ_DIR := $(BUILD_DIR)/$(BUILD)
SRCS    := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS    := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS    := $(OBJS:.o=.d)
BIN     := $(BIN_DIR)/$(BUILD)/$(TARGET)

.PHONY: all debug release run clean help
.DEFAULT_GOAL := all

all: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
	@ln -sf $(BUILD)/$(TARGET) $(BIN_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

debug:
	$(MAKE) BUILD=debug

release: all

run: $(BIN)
	./$(BIN) $(ARGS)

clean:
	$(RM) -r $(BUILD_DIR) $(BIN_DIR)

help:
	@echo "targets: all (default, release), debug, run, clean"
	@echo "vars:    BUILD=release|debug  SANITIZE=1  CXX=clang++  ARGS=\"...\""

-include $(DEPS)
