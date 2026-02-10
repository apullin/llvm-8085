#include <stdint.h>
#include <stddef.h>

/* C++ runtime stubs for freestanding environment */
extern "C" void *malloc(size_t);
extern "C" void free(void *);

void *operator new(size_t size) { return malloc(size); }
void operator delete(void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }

extern "C" void __cxa_pure_virtual() { __asm__ volatile("hlt"); }
extern "C" int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
void *__dso_handle = nullptr;

/*=== Test 1: Global constructor (static init via .init_array) ===*/
class Counter {
    volatile int value_;
public:
    Counter(int v) : value_(v) {}
    int get() const { return value_; }
    void increment() { value_ += 1; }
};

Counter g_counter(42);

/*=== Test 2: Virtual dispatch ===*/
class Shape {
public:
    virtual uint8_t sides() = 0;
    virtual ~Shape() = default;
};

class Triangle : public Shape {
public:
    uint8_t sides() override { return 3; }
};

class Square : public Shape {
public:
    uint8_t sides() override { return 4; }
};

/*=== Test 3: Template instantiation ===*/
template<typename T>
T add(T a, T b) { return a + b; }

/*=== Test 4: Dynamic allocation ===*/

extern "C" int main() {
    volatile uint8_t *out = (volatile uint8_t *)0xE000;

    /* Test 1: Global constructor ran before main */
    out[0] = (uint8_t)g_counter.get();   /* expect 0x2A (42) */
    g_counter.increment();
    out[1] = (uint8_t)g_counter.get();   /* expect 0x2B (43) */

    /* Test 2: Virtual dispatch on stack objects */
    Triangle t;
    Square s;
    Shape *shapes[2] = { &t, &s };
    out[2] = shapes[0]->sides();          /* expect 3 */
    out[3] = shapes[1]->sides();          /* expect 4 */

    /* Test 3: Template instantiation */
    out[4] = add<uint8_t>(10, 20);        /* expect 30 (0x1E) */
    uint16_t r = add<uint16_t>(100, 200);
    out[5] = (uint8_t)(r);               /* expect 44 (300 & 0xFF = 0x2C) */
    out[6] = (uint8_t)(r >> 8);          /* expect 1  (300 >> 8 = 0x01) */

    /* Test 4: Dynamic allocation + virtual destructor */
    Shape *p = new Triangle();
    out[7] = p->sides();                  /* expect 3 */
    delete p;                             /* virtual destructor */
    out[8] = 0xDD;                        /* sentinel: survived delete */

    /* Test 5: new + delete for different type */
    Shape *q = new Square();
    out[9] = q->sides();                  /* expect 4 */
    delete q;
    out[10] = 0xAA;                       /* sentinel: all tests passed */

    __asm__ volatile("hlt");
    return 0;
}
