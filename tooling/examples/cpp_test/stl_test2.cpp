// Comprehensive STL feature test for i8085 backend (C++17)
// Tests: pair, tuple, optional, string_view, numeric_limits, algorithm
//        (sort, find, min, max, reverse, count), bitset, unique_ptr,
//        initializer_list.
//
// Build: clang++ --target=i8085-unknown-elf -ffreestanding -fno-builtin
//   -fno-exceptions -fno-rtti -fno-threadsafe-statics -nostdinc++
//   -isystem $SYSROOT/include/c++/v1 -isystem $SYSROOT/include -std=c++17
//
// All 33 bytes correct at O0/O1/O2/Os.
//
// Sysroot header fixes required:
//   1. bitset: added __SIZEOF_SIZE_T__==2 case for multi-word constructor
//   2. __algorithm/sort.h: guarded extern template __sort decls and
//      library-specialized dispatch overloads with #ifndef __i8085__
//      (freestanding has no shared library providing these instantiations)
//
// Runtime fix: added hand-written memchr to stringops.S (picolibc's C
// implementation uses 32-bit word tricks that produce wrong results on i8085)
//
// Backend fixes:
//   - DAD-clobbers-carry preservation (PUSH PSW/POP PSW around DAD SP in
//     pseudo expansions when SREG live) -- fixes bitset::count() at -O1
//     and std::find pointer comparison folding.
//   - STORE_8 SrcIsHL fix: when register allocator assigns H/L as source
//     for STORE_8, copy to A before LXI clobbers HL. Fixes branchless sort
//     corruption at O2/Os.
//   - LOAD_8_WITH_ADDR OtherSubLive PUSH/POP order fix: when dest is H or L
//     and the other sub-register is live, swap PUSH order (PSW first, then H)
//     to match LIFO pop order (POP H first, then POP PSW). Fixes branchless
//     sort corruption at O1.
//   - Branchless sort now enabled for i8085 (workaround removed from sort.h).
//
// Workarounds in test code:
//   - Sort/find/count inputs copied from volatile arrays to prevent
//     compile-time constant folding
//
// NOTE: Output address is 0xE000, not 0x0200, because the i8085-64k-flat.ld
// linker script places code and data in a single flat address space. At O0
// the .text section extends well past 0x0200 (~49KB), so writing to 0x0200
// would self-modify the running code. 0xE000 is safely above code/data/heap
// and below the stack (0xFE00).
//
// Expected output at 0xE000 (33 bytes):
//   0A 14 1E 32 01 2A 00 63 05 48 02 FF 00 01 0A 14
//   1E 28 32 02 01 0A 14 03 01 03 01 03 89 4D 01 64
//   AA
//
// Byte layout:
//   [0]  pair<uint8_t,uint8_t>(10,20).first = 10 (0x0A)
//   [1]  pair.second = 20 (0x14)
//   [2]  get<0>(tuple(30,40,50)) = 30 (0x1E)
//   [3]  get<2>(tuple) = 50 (0x32)
//   [4]  optional<uint8_t>(42).has_value() = 1
//   [5]  optional.value() = 42 (0x2A)
//   [6]  nullopt.has_value() = 0
//   [7]  nullopt.value_or(99) = 99 (0x63)
//   [8]  string_view("Hello").size() = 5
//   [9]  sv[0] = 'H' (0x48)
//   [10] sv.find('l') = 2
//   [11] numeric_limits<uint8_t>::max() = 255 (0xFF)
//   [12] numeric_limits<uint8_t>::min() = 0 (0x00)
//   [13] numeric_limits<int8_t>::is_signed = 1
//   [14] sorted[0] (from {50,10,40,20,30}) = 10 (0x0A)
//   [15] sorted[1] = 20 (0x14)
//   [16] sorted[2] = 30 (0x1E)
//   [17] sorted[3] = 40 (0x28)
//   [18] sorted[4] = 50 (0x32)
//   [19] find(arr,30) result index = 2
//   [20] find(arr,99) returns end = 1
//   [21] min(10,20) = 10 (0x0A)
//   [22] max(10,20) = 20 (0x14)
//   [23] reversed[0] (from {1,2,3}) = 3
//   [24] reversed[2] = 1
//   [25] count({1,2,1,3,1}, 1) = 3
//   [26] bitset<8> test(3) = 1
//   [27] bitset::count() = 3
//   [28] bitset to_ulong() & 0xFF = 0x89
//   [29] *unique_ptr<uint8_t>(new uint8_t(77)) = 77 (0x4D)
//   [30] after release, ptr == nullptr = 1
//   [31] sum of {10,20,30,40} via initializer_list = 100 (0x64)
//   [32] 0xAA sentinel

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <utility>
#include <tuple>
#include <optional>
#include <string_view>
#include <limits>
#include <algorithm>
#include <bitset>
#include <memory>
#include <initializer_list>

// C++ runtime stubs for freestanding environment
void *operator new(size_t size) { return malloc(size); }
void *operator new(size_t size, std::align_val_t) { return malloc(size); }
void operator delete(void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }
void operator delete(void *p, size_t, std::align_val_t) noexcept { free(p); }
void operator delete(void *p, std::align_val_t) noexcept { free(p); }
extern "C" void __cxa_pure_virtual() { __asm__ volatile("hlt"); }
extern "C" int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
void *__dso_handle = nullptr;

static uint8_t sum_list(std::initializer_list<uint8_t> il) {
    uint8_t s = 0;
    for (auto v : il) s += v;
    return s;
}

extern "C" int main() {
    volatile uint8_t *out = (volatile uint8_t *)0xE000;

    // --- pair ---
    std::pair<uint8_t, uint8_t> p(10, 20);
    out[0] = p.first;          // 0x0A
    out[1] = p.second;         // 0x14

    // --- tuple ---
    {
        std::tuple<uint8_t, uint8_t, uint8_t> t((uint8_t)30, (uint8_t)40, (uint8_t)50);
        out[2] = std::get<0>(t);   // 0x1E
        out[3] = std::get<2>(t);   // 0x32
    }

    // --- optional ---
    std::optional<uint8_t> opt_val(42);
    out[4] = opt_val.has_value() ? 1 : 0;   // 1
    out[5] = opt_val.value();                // 0x2A

    std::optional<uint8_t> opt_empty;
    out[6] = opt_empty.has_value() ? 1 : 0;        // 0
    out[7] = opt_empty.value_or(99);                // 0x63

    // --- string_view ---
    std::string_view sv("Hello");
    out[8]  = (uint8_t)sv.size();        // 5
    out[9]  = (uint8_t)sv[0];           // 0x48 ('H')
    out[10] = (uint8_t)sv.find('l');    // 2

    // --- numeric_limits ---
    out[11] = std::numeric_limits<uint8_t>::max();        // 0xFF
    out[12] = std::numeric_limits<uint8_t>::min();        // 0x00
    out[13] = std::numeric_limits<int8_t>::is_signed ? 1 : 0;  // 1

    // --- algorithm: sort ---
    volatile uint8_t arr_init[] = {50, 10, 40, 20, 30};
    uint8_t arr[5];
    for (int i = 0; i < 5; i++) arr[i] = arr_init[i];
    std::sort(arr, arr + 5);
    out[14] = arr[0];   // 0x0A
    out[15] = arr[1];   // 0x14
    out[16] = arr[2];   // 0x1E
    out[17] = arr[3];   // 0x28
    out[18] = arr[4];   // 0x32

    // --- algorithm: find ---
    volatile uint8_t arr2_init[] = {10, 20, 30, 40, 50};
    uint8_t arr2[5];
    for (int i = 0; i < 5; i++) arr2[i] = arr2_init[i];
    {
        uint8_t *it = std::find(arr2, arr2 + 5, (uint8_t)30);
        out[19] = (uint8_t)(it - arr2);   // 2 (index of 30)
    }
    {
        uint8_t *it = std::find(arr2, arr2 + 5, (uint8_t)99);
        uint8_t idx = (uint8_t)(it - arr2);
        out[20] = (idx == 5) ? 1 : 0;    // 1
    }

    // --- algorithm: min/max ---
    out[21] = std::min((uint8_t)10, (uint8_t)20);   // 0x0A
    out[22] = std::max((uint8_t)10, (uint8_t)20);   // 0x14

    // --- algorithm: reverse ---
    uint8_t rev[] = {1, 2, 3};
    std::reverse(rev, rev + 3);
    out[23] = rev[0];   // 3
    out[24] = rev[2];   // 1

    // --- algorithm: count ---
    volatile uint8_t cnt_init[] = {1, 2, 1, 3, 1};
    uint8_t cnt[5];
    for (int i = 0; i < 5; i++) cnt[i] = cnt_init[i];
    out[25] = (uint8_t)std::count(cnt, cnt + 5, (uint8_t)1);  // 3

    // --- bitset ---
    std::bitset<8> bs;
    bs.set(0);
    bs.set(3);
    bs.set(7);
    out[26] = bs.test(3) ? 1 : 0;                       // 1
    out[27] = (uint8_t)bs.count();                          // 3
    out[28] = (uint8_t)(bs.to_ulong() & 0xFF);          // 0x89

    // --- unique_ptr ---
    std::unique_ptr<uint8_t> up(new uint8_t(77));
    out[29] = *up;                                       // 0x4D
    uint8_t *raw = up.release();
    volatile uint8_t released_ok = (raw != nullptr) ? 1 : 0;
    volatile uint8_t get_null = (up.get() == nullptr) ? 1 : 0;
    out[30] = (released_ok && get_null) ? 1 : 0;        // 1
    delete raw;

    // --- initializer_list ---
    out[31] = sum_list({10, 20, 30, 40});                // 0x64 (100)

    // --- sentinel ---
    out[32] = 0xAA;

    __asm__ volatile("hlt");
    return 0;
}
