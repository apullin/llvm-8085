#!/usr/bin/env python3
from pathlib import Path

TYPES = [
    ("u8",  "uint8_t",  False,  8,  0xA5,               "0xA5u"),
    ("i8",  "int8_t",   True,   8, -0x27,              "-0x27"),
    ("u16", "uint16_t", False, 16,  0xA5A5,            "0xA5A5u"),
    ("i16", "int16_t",  True,  16, -0x1234,            "-0x1234"),
    ("u32", "uint32_t", False, 32,  0x89ABCDEF,        "0x89ABCDEFu"),
    ("i32", "int32_t",  True,  32, -0x1234567,         "-0x1234567"),
    ("u64", "uint64_t", False, 64,  0x0123456789ABCDEF, "0x0123456789ABCDEFULL"),
    ("i64", "int64_t",  True,  64, -0x123456789ABCD,   "-0x123456789ABCDLL"),
]

TYPE_MAP = {t[0]: t for t in TYPES}
BASE_ADDR = 0x0200


def mask(bits):
    return (1 << bits) - 1


def cast_to(val, bits, signed):
    v = val & mask(bits)
    if signed and (v & (1 << (bits - 1))):
        v -= 1 << bits
    return v


def expected_bytes(src_name, dst_name):
    _, _, s_signed, s_bits, s_val, _ = TYPE_MAP[src_name]
    _, _, d_signed, d_bits, _, _ = TYPE_MAP[dst_name]
    s_cast = cast_to(s_val, s_bits, s_signed)
    d_cast = cast_to(s_cast, d_bits, d_signed)
    out = d_cast & mask(d_bits)
    bytes_out = []
    for i in range(d_bits // 8):
        bytes_out.append((out >> (8 * i)) & 0xFF)
    return bytes_out


def unsigned_store_type(bits):
    return {8: "uint8_t", 16: "uint16_t", 32: "uint32_t", 64: "uint64_t"}[bits]


def write_test(src_name, dst_name):
    src = TYPE_MAP[src_name]
    dst = TYPE_MAP[dst_name]

    _, src_ctype, _, _, _, src_cval = src
    _, dst_ctype, _, dst_bits, _, _ = dst

    bytes_out = expected_bytes(src_name, dst_name)
    size = len(bytes_out)

    # FileCheck lines
    lines = []
    lines.append(f"// ABI: Memory dump 0x{BASE_ADDR:04X} - 0x{BASE_ADDR + size - 1:04X} ({size} bytes):")
    bytes_str = " ".join(f"{b:02X}" for b in bytes_out)
    lines.append(f"// ABI: {BASE_ADDR:04X}: {bytes_str}")

    out_path = Path("llvm-project/llvm/test/CodeGen/I8085") / f"sim-abi-{src_name}-to-{dst_name}.c"
    with out_path.open("w", encoding="ascii") as f:
        f.write("// REQUIRES: i8085-sim\n")
        f.write("// RUN: %S/../../../../../tooling/build/build-clang-8085/bin/clang -target i8085-unknown-elf -O0 -ffreestanding -fno-builtin -nostdlib -c \\\n")
        f.write("// RUN:   %S/../../../../../sysroot/crt/crt0.S -o %t.crt0.o\n")
        f.write("// RUN: %S/../../../../../tooling/build/build-clang-8085/bin/clang -target i8085-unknown-elf -O0 -ffreestanding -fno-builtin -nostdlib -emit-llvm -c \\\n")
        f.write("// RUN:   %s -o %t.bc\n")
        f.write("// RUN: %S/../../../../../tooling/build/build-clang-8085/bin/llc -mtriple=i8085-unknown-elf -filetype=obj %t.bc -o %t.o\n")
        f.write("// RUN: %S/../../../../../tooling/build/build-clang-8085/bin/ld.lld -m i8085elf \\\n")
        f.write("// RUN:   -T %S/../../../../../sysroot/ldscripts/i8085-32kram-32krom.ld -Map %t.map \\\n")
        f.write("// RUN:   -o %t.elf %t.crt0.o %t.o\n")
        f.write("// RUN: llvm-objcopy -O binary %t.elf %t.bin\n")
        f.write(f"// RUN: %S/../../../../../i8085-trace/build/i8085-trace -S -q -n 200000 -d 0x{BASE_ADDR:04X}:0x{size:02X} %t.bin 2>&1 | FileCheck %s --check-prefix=ABI\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"static volatile {src_ctype} in_val = {src_cval};\n\n")
        f.write(f"__attribute__((noinline)) static {dst_ctype} conv({src_ctype} x) {{\n")
        f.write(f"  return ({dst_ctype})x;\n")
        f.write("}\n\n")
        f.write("int main(void) {\n")
        f.write(f"  volatile uint8_t *p = (volatile uint8_t *)0x{BASE_ADDR:04X};\n")
        f.write(f"  volatile {dst_ctype} v = conv(in_val);\n")
        if dst_bits > 32:
            f.write("  volatile uint8_t *vp = (volatile uint8_t *)&v;\n")
            for i in range(dst_bits // 8):
                f.write(f"  p[{i}] = vp[{i}];\n")
        else:
            f.write(f"  {unsigned_store_type(dst_bits)} u = ({unsigned_store_type(dst_bits)})v;\n")
            for i in range(dst_bits // 8):
                f.write(f"  p[{i}] = (uint8_t)(u >> {8 * i});\n")
        f.write("  return 0;\n")
        f.write("}\n\n")
        for line in lines:
            f.write(line + "\n")
        f.write("// ABI: \"halt\":\"hlt\"\n")

    print(f"wrote {out_path}")


def main():
    for src_name, *_ in TYPES:
        for dst_name, *_ in TYPES:
            write_test(src_name, dst_name)


if __name__ == "__main__":
    main()
