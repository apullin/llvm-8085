# FreeRTOS Intel 8080 port - Makefile
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
             -I $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8080
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

# Heap allocator (heap_4 = coalescing malloc/free)
HEAP_SRC := \
    $(ROOT)/FreeRTOS-Kernel/portable/MemMang/heap_4.c

# Event groups
EVENT_SRC := \
    $(ROOT)/FreeRTOS-Kernel/event_groups.c

# Port sources (8080)
PORT_SRC := \
    $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8080/port.c

PORT_ASM := \
    $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8080/portasm.S

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
HEAP_OBJS       := $(patsubst $(ROOT)/FreeRTOS-Kernel/portable/MemMang/%.c,$(BUILDDIR)/heap/%.o,$(HEAP_SRC))
EVENT_OBJS      := $(patsubst $(ROOT)/FreeRTOS-Kernel/%.c,$(BUILDDIR)/kernel/%.o,$(EVENT_SRC))
PORT_C_OBJS     := $(patsubst $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8080/%.c,$(BUILDDIR)/port/%.o,$(PORT_SRC))
PORT_ASM_OBJS   := $(patsubst $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8080/%.S,$(BUILDDIR)/port/%.o,$(PORT_ASM))
STARTUP_OBJS    := $(patsubst %.S,$(BUILDDIR)/%.o,$(STARTUP_SRC))
DEMO_BASIC_OBJ  := $(BUILDDIR)/demo_basic.o

ALL_BASIC_OBJS  := $(STARTUP_OBJS) $(KERNEL_OBJS) $(HEAP_OBJS) $(PORT_C_OBJS) $(PORT_ASM_OBJS) $(DEMO_BASIC_OBJ)

.PHONY: all clean run

all: $(DEMO_BASIC_BIN) $(DEMO_BASIC_LIST)

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

# Compile heap allocator
$(BUILDDIR)/heap/%.o: $(ROOT)/FreeRTOS-Kernel/portable/MemMang/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile FreeRTOS kernel C sources (tasks.c, list.c, queue.c)
$(BUILDDIR)/kernel/%.o: $(ROOT)/FreeRTOS-Kernel/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile port C sources
$(BUILDDIR)/port/%.o: $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8080/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble port assembly
$(BUILDDIR)/port/%.o: $(ROOT)/FreeRTOS-Kernel/portable/GCC/I8080/%.S
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

# Run basic two-task test
# RST 7 period = CPU_CLOCK / TICK_RATE = 2000000 / 100 = 20000 cycles
# On the 8080, the external 8253 timer + 8259 PIC fires RST 7 at
# regular intervals.  The simulator's --timer flag simulates this.
run: $(DEMO_BASIC_BIN)
	$(TRACE) \
	    --timer=7:20000 \
	    -n 500000 -S \
	    -d 0xFE00:8 \
	    $(DEMO_BASIC_BIN)
