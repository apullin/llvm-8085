// C++ codegen stress test for i8085 backend
// Tests: lambdas with captures, multiple inheritance, move semantics,
//        variadic templates, structured bindings, std::function,
//        deep inheritance chains.
//
// Build: clang++ --target=i8085-unknown-elf -ffreestanding -fno-builtin
//   -fno-exceptions -fno-rtti -fno-threadsafe-statics -nostdinc++
//   -isystem $SYSROOT/include/c++/v1 -isystem $SYSROOT/include -std=c++17
//
// Expected output at 0xE000 (40 bytes):
//   See byte layout below.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <utility>
#include <tuple>
#include <initializer_list>

// C++ runtime stubs for freestanding environment
extern "C" void *malloc(size_t);
extern "C" void free(void *);
void *operator new(size_t size) { return malloc(size); }
void *operator new(size_t size, std::align_val_t) { return malloc(size); }
void operator delete(void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }
void operator delete(void *p, size_t, std::align_val_t) noexcept { free(p); }
void operator delete(void *p, std::align_val_t) noexcept { free(p); }
extern "C" void __cxa_pure_virtual() { __asm__ volatile("hlt"); }
extern "C" int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
void *__dso_handle = nullptr;

static volatile uint8_t *out = (volatile uint8_t *)0xE000;
static volatile uint8_t byte_idx = 0;

static void emit(uint8_t v) {
    out[byte_idx] = v;
    byte_idx++;
}

// =========================================================================
// 1. Lambdas with captures
// =========================================================================

// Capture by value
static uint8_t test_lambda_val() {
    uint8_t a = 10, b = 20;
    auto add = [a, b]() -> uint8_t { return a + b; };
    return add();  // expect 30
}

// Capture by reference
static uint8_t test_lambda_ref() {
    uint8_t x = 5;
    auto triple = [&x]() { x = x * 3; };
    triple();
    return x;  // expect 15
}

// Capture by value with mutable
static uint8_t test_lambda_mutable() {
    uint8_t counter = 0;
    auto inc = [counter]() mutable -> uint8_t {
        counter++;
        return counter;
    };
    inc();
    inc();
    return inc();  // expect 3 (each call sees its own copy's state)
}

// Lambda passed as argument (function pointer-like)
template<typename F>
static uint8_t apply(F f, uint8_t x) {
    return f(x);
}

static uint8_t test_lambda_arg() {
    uint8_t offset = 100;
    auto add_offset = [offset](uint8_t x) -> uint8_t { return x + offset; };
    return apply(add_offset, 42);  // expect 142
}

// =========================================================================
// 2. Multiple inheritance
// =========================================================================

struct HasID {
    uint8_t id;
    HasID(uint8_t i) : id(i) {}
    uint8_t get_id() { return id; }
};

struct HasValue {
    uint8_t value;
    HasValue(uint8_t v) : value(v) {}
    uint8_t get_value() { return value; }
};

struct Widget : HasID, HasValue {
    Widget(uint8_t i, uint8_t v) : HasID(i), HasValue(v) {}
    uint8_t combined() { return get_id() + get_value(); }
};

static void test_multiple_inheritance() {
    Widget w(7, 33);
    emit(w.get_id());      // [4] = 7
    emit(w.get_value());   // [5] = 33 (0x21)
    emit(w.combined());    // [6] = 40 (0x28)

    // Pointer adjustment: cast to each base
    HasID *pid = &w;
    HasValue *pval = &w;
    emit(pid->get_id());        // [7] = 7
    emit(pval->get_value());    // [8] = 33 (0x21)
}

// =========================================================================
// 3. Move semantics
// =========================================================================

struct OwnedBuffer {
    uint8_t *data;
    uint8_t size;

    OwnedBuffer(uint8_t sz) : size(sz) {
        data = (uint8_t *)malloc(sz);
        for (uint8_t i = 0; i < sz; i++) data[i] = i + 1;
    }
    ~OwnedBuffer() {
        if (data) free(data);
    }

    // Move constructor
    OwnedBuffer(OwnedBuffer &&other) : data(other.data), size(other.size) {
        other.data = nullptr;
        other.size = 0;
    }

    // Move assignment
    OwnedBuffer &operator=(OwnedBuffer &&other) {
        if (this != &other) {
            if (data) free(data);
            data = other.data;
            size = other.size;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }

    // Delete copy
    OwnedBuffer(const OwnedBuffer &) = delete;
    OwnedBuffer &operator=(const OwnedBuffer &) = delete;
};

static void test_move_semantics() {
    OwnedBuffer a(3);
    emit(a.data[0]);   // [9] = 1
    emit(a.data[2]);   // [10] = 3
    emit(a.size);      // [11] = 3

    // Move construct
    OwnedBuffer b(std::move(a));
    emit(a.data == nullptr ? 1 : 0);  // [12] = 1 (a moved from)
    emit(a.size);                      // [13] = 0
    emit(b.data[0]);                   // [14] = 1
    emit(b.size);                      // [15] = 3

    // Move assign
    OwnedBuffer c(2);
    c = std::move(b);
    emit(b.data == nullptr ? 1 : 0);  // [16] = 1 (b moved from)
    emit(c.data[1]);                   // [17] = 2
    emit(c.size);                      // [18] = 3
}

// =========================================================================
// 4. Variadic templates
// =========================================================================

// Sum of variadic args
static uint8_t vsum() { return 0; }

template<typename T, typename... Rest>
static uint8_t vsum(T first, Rest... rest) {
    return (uint8_t)first + vsum(rest...);
}

// Count of args
static uint8_t vcount() { return 0; }

template<typename T, typename... Rest>
static uint8_t vcount(T, Rest... rest) {
    return 1 + vcount(rest...);
}

static void test_variadic_templates() {
    emit(vsum((uint8_t)1, (uint8_t)2, (uint8_t)3, (uint8_t)4));  // [19] = 10
    emit(vcount(1, 2, 3, 4, 5));                                   // [20] = 5
}

// =========================================================================
// 5. Structured bindings (C++17)
// =========================================================================

struct Point {
    uint8_t x, y;
};

static Point make_point(uint8_t x, uint8_t y) {
    return {x, y};
}

static void test_structured_bindings() {
    // Structured binding from pair
    auto [a, b] = std::make_pair((uint8_t)11, (uint8_t)22);
    emit(a);   // [21] = 11 (0x0B)
    emit(b);   // [22] = 22 (0x16)

    // Structured binding from tuple
    auto [x, y, z] = std::make_tuple((uint8_t)5, (uint8_t)10, (uint8_t)15);
    emit(x);   // [23] = 5
    emit(y);   // [24] = 10
    emit(z);   // [25] = 15

    // Structured binding from struct
    auto [px, py] = make_point(44, 55);
    emit(px);  // [26] = 44 (0x2C)
    emit(py);  // [27] = 55 (0x37)
}

// =========================================================================
// 6. Deep inheritance chain with virtual methods
// =========================================================================

struct Base {
    virtual uint8_t level() { return 0; }
    virtual uint8_t compute(uint8_t x) { return x; }
    virtual ~Base() = default;
};

struct Level1 : Base {
    uint8_t level() override { return 1; }
    uint8_t compute(uint8_t x) override { return x + 10; }
};

struct Level2 : Level1 {
    uint8_t level() override { return 2; }
    uint8_t compute(uint8_t x) override { return Level1::compute(x) + 20; }
};

struct Level3 : Level2 {
    uint8_t level() override { return 3; }
    uint8_t compute(uint8_t x) override { return Level2::compute(x) + 30; }
};

static void test_deep_inheritance() {
    Level3 obj;
    Base *bp = &obj;
    emit(bp->level());          // [28] = 3
    emit(bp->compute(5));       // [29] = 65 (5+10+20+30)

    // Dynamic type through pointer to intermediate
    Level1 *l1p = &obj;
    emit(l1p->level());         // [30] = 3 (still dispatches to Level3)
    emit(l1p->compute(1));      // [31] = 61 (1+10+20+30)
}

// =========================================================================
// 7. Type-erased callable (manual std::function-like)
// =========================================================================
// (Avoids #include <functional> which pulls in heavy headers.
//  Tests the same codegen patterns: vtable, heap alloc, type erasure.)

struct ICallable {
    virtual uint8_t call(uint8_t x) = 0;
    virtual ~ICallable() = default;
};

template<typename F>
struct CallableImpl : ICallable {
    F func;
    CallableImpl(F f) : func(f) {}
    uint8_t call(uint8_t x) override { return func(x); }
};

struct SimpleFunction {
    ICallable *impl;

    template<typename F>
    SimpleFunction(F f) : impl(new CallableImpl<F>(f)) {}

    ~SimpleFunction() { delete impl; }

    uint8_t operator()(uint8_t x) { return impl->call(x); }

    // Move only
    SimpleFunction(SimpleFunction &&other) : impl(other.impl) {
        other.impl = nullptr;
    }
    SimpleFunction(const SimpleFunction &) = delete;
    SimpleFunction &operator=(const SimpleFunction &) = delete;
};

static void test_type_erasure() {
    uint8_t base = 50;

    // Lambda with capture, type-erased through virtual dispatch
    SimpleFunction fn([base](uint8_t x) -> uint8_t {
        return base + x;
    });
    emit(fn(7));   // [32] = 57 (0x39)
    emit(fn(13));  // [33] = 63 (0x3F)

    // Move the function object
    SimpleFunction fn2(std::move(fn));
    emit(fn.impl == nullptr ? 1 : 0);  // [34] = 1 (moved from)
    emit(fn2(20));                       // [35] = 70 (0x46)
}

// =========================================================================
// 8. Lambda as comparator (combines lambda + template + algorithm pattern)
// =========================================================================

template<typename T, typename Cmp>
static void insertion_sort(T *arr, uint8_t n, Cmp cmp) {
    for (uint8_t i = 1; i < n; i++) {
        T key = arr[i];
        uint8_t j = i;
        while (j > 0 && cmp(key, arr[j - 1])) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = key;
    }
}

static void test_lambda_comparator() {
    volatile uint8_t init[] = {30, 10, 50, 20, 40};
    uint8_t arr[5];
    for (int i = 0; i < 5; i++) arr[i] = init[i];

    // Sort descending using lambda comparator
    insertion_sort(arr, 5, [](uint8_t a, uint8_t b) -> bool {
        return a > b;
    });
    emit(arr[0]);  // [36] = 50 (0x32) — largest first
    emit(arr[4]);  // [37] = 10 (0x0A) — smallest last
}

// =========================================================================

extern "C" int main() {
    byte_idx = 0;

    // 1. Lambdas
    emit(test_lambda_val());       // [0] = 30 (0x1E)
    emit(test_lambda_ref());       // [1] = 15 (0x0F)
    emit(test_lambda_mutable());   // [2] = 3
    emit(test_lambda_arg());       // [3] = 142 (0x8E)

    // 2. Multiple inheritance
    test_multiple_inheritance();   // [4..8]

    // 3. Move semantics
    test_move_semantics();         // [9..18]

    // 4. Variadic templates
    test_variadic_templates();     // [19..20]

    // 5. Structured bindings
    test_structured_bindings();    // [21..27]

    // 6. Deep inheritance
    test_deep_inheritance();       // [28..31]

    // 7. Type erasure
    test_type_erasure();           // [32..35]

    // 8. Lambda comparator
    test_lambda_comparator();      // [36..37]

    // Sentinel
    emit(0xBE);   // [38]
    emit(0xEF);   // [39]

    __asm__ volatile("hlt");
    return 0;
}
