TARGET    := packet-analyzer
SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build
BIN_DIR   := bin

# PcapPlusPlus lives in a git submodule and is built static into a local
# prefix, so nothing needs installing system wide.
CMAKE         ?= cmake
PCAPPP_DIR    := third_party/PcapPlusPlus
PCAPPP_BUILD  := third_party/build/PcapPlusPlus
PCAPPP_PREFIX := $(abspath third_party/install)
PCAPPP_LIB    := $(PCAPPP_PREFIX)/lib/libPcap++.a

PCAPPP_CMAKE_FLAGS := \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DPCAPPP_BUILD_TESTS=OFF \
    -DPCAPPP_BUILD_EXAMPLES=OFF \
    -DPCAPPP_BUILD_FUZZERS=OFF \
    -DPCAPPP_INSTALL=ON \
    -DCMAKE_INSTALL_PREFIX=$(PCAPPP_PREFIX)

CXX      ?= g++
CXXSTD   := -std=c++23
WARNINGS := -Wall -Wextra -Wpedantic
CPPFLAGS += -I$(SRC_DIR) -I$(INC_DIR) -MMD -MP
# -isystem so their headers do not answer to our warning flags. Include them as
# <pcapplusplus/Packet.h> -- the bare name collides with our own core/Packet.h.
CPPFLAGS += -isystem $(PCAPPP_PREFIX)/include
CXXFLAGS += $(CXXSTD) $(WARNINGS)
# Static archives resolve left to right, so Pcap++ must precede what it needs.
LDLIBS   += -L$(PCAPPP_PREFIX)/lib -lPcap++ -lPacket++ -lCommon++
LDLIBS   += -lpcap -lpthread -lftxui-component -lftxui-dom -lftxui-screen

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

.PHONY: all debug release run clean distclean help pcapplusplus
.DEFAULT_GOAL := all

all: $(BIN)

$(BIN): $(OBJS) | $(PCAPPP_LIB)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	@ln -sf $(BUILD)/$(TARGET) $(BIN_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(PCAPPP_LIB)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

pcapplusplus: $(PCAPPP_LIB)

$(PCAPPP_LIB): $(PCAPPP_DIR)/CMakeLists.txt
	$(CMAKE) -S $(PCAPPP_DIR) -B $(PCAPPP_BUILD) $(PCAPPP_CMAKE_FLAGS)
	$(CMAKE) --build $(PCAPPP_BUILD) --parallel
	$(CMAKE) --install $(PCAPPP_BUILD)

$(PCAPPP_DIR)/CMakeLists.txt:
	@echo "PcapPlusPlus submodule is not checked out. Run:"
	@echo "    git submodule update --init --recursive"
	@exit 1

debug:
	$(MAKE) BUILD=debug

release: all

run: $(BIN)
	./$(BIN) $(ARGS)

# Leaves the submodule build alone -- rebuilding it is slow and it rarely moves.
clean:
	$(RM) -r $(BUILD_DIR) $(BIN_DIR)

distclean: clean
	$(RM) -r $(PCAPPP_BUILD) $(PCAPPP_PREFIX)

help:
	@echo "targets: all (default, release), debug, run, clean, distclean, pcapplusplus"
	@echo "vars:    BUILD=release|debug  SANITIZE=1  CXX=clang++  ARGS=\"...\""

-include $(DEPS)
