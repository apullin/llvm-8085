# Makefile for compiler-rt builtin unit tests on i8085
#
# Builds and runs each test against the i8085 simulator.

ROOT ?= $(shell cd ../../.. && pwd)

# Toolchain
CLANG   ?= $(ROOT)/llvm-project/build-clang-8085/bin/clang
LLD     ?= $(ROOT)/llvm-project/build-clang-8085/bin/ld.lld
OBJCOPY ?= $(ROOT)/llvm-project/build-clang-8085/bin/llvm-objcopy
SIZE    ?= $(ROOT)/llvm-project/build-clang-8085/bin/llvm-size
TRACE   ?= $(ROOT)/i8085-trace/build/i8085-trace

# Sysroot
CRT     ?= $(ROOT)/sysroot/crt/crt0.S
LIBGCC  ?= $(ROOT)/sysroot/lib/libgcc.a
LIBC    ?= $(ROOT)/sysroot/lib/libc.a
LDSCRIPT ?= $(ROOT)/sysroot/ldscripts/i8085-16kram-48krom.ld

# Flags
OPT      ?= Oz
CFLAGS    = --target=i8085-unknown-elf -ffreestanding -fno-builtin -$(OPT) -I.
LDFLAGS   = -m i8085elf --gc-sections -T $(LDSCRIPT)

# Test programs
TESTS = rt_test_mulsi3 rt_test_divsi3 rt_test_float_arith rt_test_float_conv rt_test_arith64

# Simulator settings per test
MAX_STEPS_rt_test_mulsi3       = 5000000
MAX_STEPS_rt_test_divsi3       = 50000000
MAX_STEPS_rt_test_float_arith  = 20000000
MAX_STEPS_rt_test_float_conv   = 20000000
MAX_STEPS_rt_test_arith64      = 100000000

BUILDDIR = build/$(OPT)

.PHONY: all clean run

all: $(foreach t,$(TESTS),$(BUILDDIR)/$(t).bin)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Compile CRT
$(BUILDDIR)/crt0.o: $(CRT) | $(BUILDDIR)
	$(CLANG) --target=i8085-unknown-elf -ffreestanding -fno-builtin -$(OPT) -c $< -o $@

# Compile test source
$(BUILDDIR)/%.o: %.c rt_test.h | $(BUILDDIR)
	$(CLANG) $(CFLAGS) -c $< -o $@

# Link
$(BUILDDIR)/%.elf: $(BUILDDIR)/%.o $(BUILDDIR)/crt0.o $(LIBGCC) $(LIBC)
	$(LLD) $(LDFLAGS) -o $@ $(BUILDDIR)/crt0.o $< $(LIBGCC) $(LIBC) $(LIBGCC)

# Create binary
$(BUILDDIR)/%.bin: $(BUILDDIR)/%.elf
	$(OBJCOPY) -O binary $< $@

# Run a single test: make run-rt_test_mulsi3
run-%: $(BUILDDIR)/%.bin
	@echo "=== Running $* ($(OPT)) ==="
	@$(TRACE) -e 0x0000 -l 0x0000 -n $(MAX_STEPS_$*) -S -q -d 0x0200:8 $< 2>&1 | \
		{ read json; echo "$$json"; }

# Run all tests
run: $(foreach t,$(TESTS),run-$(t))

clean:
	rm -rf build
