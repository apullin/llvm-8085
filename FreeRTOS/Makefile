# FreeRTOS Intel 8085 port - Makefile
#
# Build:  make                    (builds basic two-task test)
# Clean:  make clean
# Test:   make run                (basic two-task test)

ROOT      := $(shell cd .. && pwd)
LLVM_BIN  := $(ROOT)/llvm-project/build-clang-8085/bin
CC        := $(LLVM_BIN)/clang
AS        := $(CC)
LD        := $(LLVM_BIN)/ld.lld
OBJCOPY   := $(LLVM_BIN)/llvm-objcopy
OBJDUMP   := $(LLVM_BIN)/llvm-objdump
TRACE     := $(ROOT)/i8085-trace/build/i8085-trace

TARGET    := --target=i8085-unknown-elf
SYSROOT   := $(ROOT)/sysroot
CLANG_RES := $(ROOT)/llvm-project/build-clang-8085/lib/clang/20/i8085/include

OPT       ?= -Oz
CFLAGS    := $(TARGET) $(OPT) -ffreestanding -fno-builtin -nostdlib \
             -ffunction-sections -fdata-sections \
             -isystem $(CLANG_RES) \
             -isystem $(SYSROOT)/include \
             -I . \
             -I $(ROOT)/FreeRTOS-Kernel/include \
             -I $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8085
ASFLAGS   := $(TARGET) -c
LDFLAGS   := -m i8085elf -T linker.ld --gc-sections

LIBGCC    := $(SYSROOT)/lib/libgcc.a
LIBC      := $(SYSROOT)/lib/libc.a

BUILDDIR  := build

# FreeRTOS kernel sources (minimal set)
KERNEL_SRC := \
    $(ROOT)/FreeRTOS-Kernel/tasks.c \
    $(ROOT)/FreeRTOS-Kernel/list.c \
    $(ROOT)/FreeRTOS-Kernel/queue.c

# Port sources
PORT_SRC := \
    $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8085/port.c

PORT_ASM := \
    $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8085/portasm.S

# Startup
STARTUP_SRC := \
    startup.S

# ---- Basic two-task demo ----
DEMO_BASIC_ELF  := $(BUILDDIR)/freertos_basic.elf
DEMO_BASIC_BIN  := $(BUILDDIR)/freertos_basic.bin
DEMO_BASIC_MAP  := $(BUILDDIR)/freertos_basic.map
DEMO_BASIC_LIST := $(BUILDDIR)/freertos_basic.list

# Object files
KERNEL_OBJS     := $(patsubst $(ROOT)/FreeRTOS-Kernel/%.c,$(BUILDDIR)/kernel/%.o,$(KERNEL_SRC))
PORT_C_OBJS     := $(patsubst $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8085/%.c,$(BUILDDIR)/port/%.o,$(PORT_SRC))
PORT_ASM_OBJS   := $(patsubst $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8085/%.S,$(BUILDDIR)/port/%.o,$(PORT_ASM))
STARTUP_OBJS    := $(patsubst %.S,$(BUILDDIR)/%.o,$(STARTUP_SRC))
DEMO_BASIC_OBJ  := $(BUILDDIR)/demo_basic.o

ALL_BASIC_OBJS  := $(STARTUP_OBJS) $(KERNEL_OBJS) $(PORT_C_OBJS) $(PORT_ASM_OBJS) $(DEMO_BASIC_OBJ)

# ---- Queue demo ----
DEMO_QUEUE_ELF  := $(BUILDDIR)/freertos_queue.elf
DEMO_QUEUE_BIN  := $(BUILDDIR)/freertos_queue.bin
DEMO_QUEUE_MAP  := $(BUILDDIR)/freertos_queue.map
DEMO_QUEUE_LIST := $(BUILDDIR)/freertos_queue.list
DEMO_QUEUE_OBJ  := $(BUILDDIR)/demo_queue.o

ALL_QUEUE_OBJS  := $(STARTUP_OBJS) $(KERNEL_OBJS) $(PORT_C_OBJS) $(PORT_ASM_OBJS) $(DEMO_QUEUE_OBJ)

.PHONY: all clean run run-queue queue

all: $(DEMO_BASIC_BIN) $(DEMO_BASIC_LIST)

queue: $(DEMO_QUEUE_BIN) $(DEMO_QUEUE_LIST)

# Binary extraction
$(BUILDDIR)/%.bin: $(BUILDDIR)/%.elf
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/%.list: $(BUILDDIR)/%.elf
	$(OBJDUMP) -d $< > $@

# Link basic demo
$(DEMO_BASIC_ELF): $(ALL_BASIC_OBJS) linker.ld
	$(LD) $(LDFLAGS) -Map $(DEMO_BASIC_MAP) \
	    $(ALL_BASIC_OBJS) $(LIBGCC) $(LIBC) $(LIBGCC) \
	    -o $@

# Compile FreeRTOS kernel C sources
# Previously forced -O0 due to a codegen bug where Machine Copy Propagation
# incorrectly removed COPY $hl,$bc when pseudo instructions declared Defs=[HL].
# Fixed by removing HL from Defs of STORE/LOAD pseudos (they conditionally
# preserve HL via PUSH/POP during expansion).
$(BUILDDIR)/kernel/%.o: $(ROOT)/FreeRTOS-Kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile port C sources
$(BUILDDIR)/port/%.o: $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8085/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble port assembly
$(BUILDDIR)/port/%.o: $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8085/%.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Compile demo C sources
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup
$(BUILDDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)

# Link queue demo
$(DEMO_QUEUE_ELF): $(ALL_QUEUE_OBJS) linker.ld
	$(LD) $(LDFLAGS) -Map $(DEMO_QUEUE_MAP) \
	    $(ALL_QUEUE_OBJS) $(LIBGCC) $(LIBC) $(LIBGCC) \
	    -o $@

# Run basic two-task test
# RST 6.5 period = CPU_CLOCK / TICK_RATE = 3072000 / 100 = 30720 cycles
run: $(DEMO_BASIC_BIN)
	$(TRACE) \
	    --timer=65:30720 \
	    -n 500000 -S \
	    -d 0xFE00:8 \
	    $(DEMO_BASIC_BIN)

# Run queue demo
run-queue: $(DEMO_QUEUE_BIN)
	$(TRACE) \
	    --timer=65:30720 \
	    -n 500000 -S \
	    -d 0xFE00:8 \
	    $(DEMO_QUEUE_BIN)
