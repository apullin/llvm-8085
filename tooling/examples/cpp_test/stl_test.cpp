// Comprehensive STL test for i8085 backend
// Tests std::array, std::vector, and std::string on a freestanding 8-bit target.
//
// Expected output at 0xE000 (27 bytes):
//   0A 14 1E 28 04 03 0A 14 1E 02 0A 1E 00 05 48 6F
//   06 21 01 00 48 21 04 54 74 BB AA
//
// Byte layout:
//   [0]  array[0] = 10 (0x0A)
//   [1]  array[1] = 20 (0x14)
//   [2]  array[2] = 30 (0x1E)
//   [3]  array[3] = 40 (0x28)
//   [4]  array.size() = 4 (0x04)
//   [5]  vec.size() after 3 push_backs = 3 (0x03)
//   [6]  vec[0] = 10 (0x0A)
//   [7]  vec[1] = 20 (0x14)
//   [8]  vec[2] = 30 (0x1E)
//   [9]  vec.size() after erase(begin+1) = 2 (0x02)
//   [10] vec[0] after erase = 10 (0x0A)
//   [11] vec[1] after erase = 30 (0x1E)
//   [12] vec.size() after clear = 0 (0x00)
//   [13] str.size() = 5 ("Hello") (0x05)
//   [14] str[0] = 'H' (0x48)
//   [15] str[4] = 'o' (0x6F)
//   [16] str.size() after += "!" = 6 (0x06)
//   [17] str[5] = '!' (0x21)
//   [18] (str == "Hello!") ? 1 : 0 = 1 (0x01)
//   [19] (str == "World") ? 1 : 0 = 0 (0x00)
//   [20] str.data()[0] = 'H' (0x48)
//   [21] str.data()[5] = '!' (0x21)
//   [22] str2("Test").size() = 4 (0x04)
//   [23] str2[0] = 'T' (0x54)
//   [24] str2[3] = 't' (0x74)
//   [25] 0xBB (string tests done marker)
//   [26] 0xAA (sentinel: all tests passed)
//
// Build: clang++ --target=i8085-unknown-elf -ffreestanding -fno-builtin
//   -fno-exceptions -fno-rtti -fno-threadsafe-statics -nostdinc++
//   -isystem $SYSROOT/include/c++/v1 -isystem $SYSROOT/include -O0
//
// All 27 bytes correct at O0/O1/O2/Os.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <array>
#include <vector>
#include <string>

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

extern "C" int main() {
    volatile uint8_t *out = (volatile uint8_t *)0xE000;

    // ===== std::array tests =====
    std::array<uint8_t, 4> arr = {10, 20, 30, 40};
    out[0] = arr[0];            // expect 0x0A
    out[1] = arr[1];            // expect 0x14
    out[2] = arr[2];            // expect 0x1E
    out[3] = arr[3];            // expect 0x28
    out[4] = arr.size();        // expect 0x04

    // ===== std::vector tests =====
    std::vector<uint8_t> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    out[5] = vec.size();        // expect 0x03
    out[6] = vec[0];            // expect 0x0A
    out[7] = vec[1];            // expect 0x14
    out[8] = vec[2];            // expect 0x1E

    // Erase middle element (20)
    vec.erase(vec.begin() + 1);
    out[9]  = vec.size();       // expect 0x02
    out[10] = vec[0];           // expect 0x0A
    out[11] = vec[1];           // expect 0x1E

    // Clear
    vec.clear();
    out[12] = vec.size();       // expect 0x00

    // ===== std::string tests =====
    std::string str("Hello");
    out[13] = str.size();       // expect 0x05
    out[14] = str[0];           // expect 0x48 ('H')
    out[15] = str[4];           // expect 0x6F ('o')

    // Append
    str += "!";
    out[16] = str.size();       // expect 0x06
    out[17] = str[5];           // expect 0x21 ('!')

    // Comparison
    out[18] = (str == "Hello!") ? 1 : 0;   // expect 0x01
    out[19] = (str == "World")  ? 1 : 0;   // expect 0x00

    // Data access
    const char *p = str.data();
    out[20] = p[0];             // expect 0x48 ('H')
    out[21] = p[5];             // expect 0x21 ('!')

    // Second string
    std::string str2("Test");
    out[22] = str2.size();      // expect 0x04
    out[23] = str2[0];          // expect 0x54 ('T')
    out[24] = str2[3];          // expect 0x74 ('t')

    // Markers
    out[25] = 0xBB;             // string tests done
    out[26] = 0xAA;             // sentinel: all tests passed

    __asm__ volatile("hlt");
    return 0;
}
