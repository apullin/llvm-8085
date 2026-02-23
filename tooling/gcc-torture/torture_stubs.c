/*
 * Stubs for GCC C torture test suite on i8085 simulator.
 *
 * The torture tests call abort() on failure, exit(0) on success.
 * Includes a minimal printf/sprintf implementation (picolibc's full
 * printf exceeds the 64KB address space on 8085).
 *
 * We write a status marker to 0xFE00 before halting so the
 * harness can distinguish abort (failure) from normal exit.
 *
 * Memory map:
 *   0xFE00: status word (little-endian)
 *     0x0000 = exit(0) = PASS
 *     0xDEAD = abort() = FAIL
 *     other  = exit(n)
 */

#include <stdarg.h>

/* Status port at fixed memory address */
#define STATUS_ADDR ((volatile unsigned int *)0xFE00)

void abort(void) {
    *STATUS_ADDR = 0xDEADu;
    __asm__ volatile("hlt");
    __builtin_unreachable();
}

void exit(int status) {
    *STATUS_ADDR = (unsigned int)status;
    __asm__ volatile("hlt");
    __builtin_unreachable();
}

/* __builtin_exit -- GCC extension, same as exit */
void __builtin_exit(int status) {
    exit(status);
}

/* signal() stub -- i8085 has no hardware traps. */
typedef void (*sighandler_t)(int);
sighandler_t signal(int sig, sighandler_t handler) {
    (void)sig;
    (void)handler;
    return (sighandler_t)0;
}

/* raise() stub */
int raise(int sig) {
    (void)sig;
    return 0;
}

/* fflush stub */
int fflush(void *stream) {
    (void)stream;
    return 0;
}

/* stdout/stderr stubs for tests that reference them */
void *stdout = (void *)0;
void *stderr = (void *)0;

/* ================================================================
 * Minimal printf/sprintf/snprintf/fprintf for torture tests.
 *
 * Supported: %d %i %u %ld %li %lu %x %X %lx %lX %o %lo
 *            %s %c %% %p
 * Flags: # 0 - + (space)
 * Width and precision
 * Length modifiers: h hh l
 * Float: %.Nf (basic)
 * ================================================================ */

/* Output function type: writes char, returns 1 on success */
typedef struct {
    char *buf;
    int pos;
    int max;  /* -1 for unbounded (printf to stdout) */
} out_state;

static void out_char(out_state *s, char c) {
    if (s->max < 0 || s->pos < s->max - 1) {
        if (s->buf)
            s->buf[s->pos] = c;
    }
    s->pos++;
}

static void out_string(out_state *s, const char *str) {
    while (*str)
        out_char(s, *str++);
}

static void out_pad(out_state *s, char c, int count) {
    while (count-- > 0)
        out_char(s, c);
}

/* Reverse a string in place */
static void reverse(char *s, int len) {
    int i = 0, j = len - 1;
    while (i < j) {
        char t = s[i];
        s[i] = s[j];
        s[j] = t;
        i++;
        j--;
    }
}

/* Convert unsigned long to string in given base, return length */
static int utoa_base(unsigned long val, char *buf, int base, int uppercase) {
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int len = 0;
    if (val == 0) {
        buf[len++] = '0';
    } else {
        while (val > 0) {
            buf[len++] = digits[val % base];
            val /= base;
        }
    }
    buf[len] = '\0';
    reverse(buf, len);
    return len;
}

/* Format and output a single conversion specifier */
static void format_arg(out_state *s, const char **pfmt, va_list *pap) {
    const char *fmt = *pfmt;
    char numbuf[34];  /* enough for 32-bit in binary + sign + prefix */
    int numlen;

    /* Parse flags */
    int flag_minus = 0, flag_plus = 0, flag_space = 0;
    int flag_zero = 0, flag_hash = 0;

    for (;;) {
        switch (*fmt) {
            case '-': flag_minus = 1; fmt++; continue;
            case '+': flag_plus = 1; fmt++; continue;
            case ' ': flag_space = 1; fmt++; continue;
            case '0': flag_zero = 1; fmt++; continue;
            case '#': flag_hash = 1; fmt++; continue;
        }
        break;
    }

    /* Parse width */
    int width = 0;
    int has_width = 0;
    if (*fmt == '*') {
        width = va_arg(*pap, int);
        has_width = 1;
        if (width < 0) {
            flag_minus = 1;
            width = -width;
        }
        fmt++;
    } else {
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            has_width = 1;
            fmt++;
        }
    }

    /* Parse precision */
    int precision = -1;
    if (*fmt == '.') {
        fmt++;
        precision = 0;
        if (*fmt == '*') {
            precision = va_arg(*pap, int);
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                precision = precision * 10 + (*fmt - '0');
                fmt++;
            }
        }
    }

    /* Parse length modifier */
    int len_long = 0;
    int len_short = 0;
    int len_char = 0;
    if (*fmt == 'l') {
        len_long = 1;
        fmt++;
        if (*fmt == 'l') { fmt++; } /* ll = same as l on i8085 */
    } else if (*fmt == 'h') {
        len_short = 1;
        fmt++;
        if (*fmt == 'h') {
            len_char = 1;
            len_short = 0;
            fmt++;
        }
    } else if (*fmt == 'z' || *fmt == 't') {
        fmt++;  /* size_t/ptrdiff_t = int on i8085 */
    }

    /* Conversion specifier */
    char spec = *fmt++;
    *pfmt = fmt;

    switch (spec) {
    case 'd':
    case 'i': {
        long val;
        if (len_long)
            val = va_arg(*pap, long);
        else
            val = (long)va_arg(*pap, int);
        if (len_char)
            val = (signed char)val;
        else if (len_short)
            val = (short)val;

        char sign = 0;
        unsigned long uval;
        if (val < 0) {
            sign = '-';
            uval = (unsigned long)(-val);
        } else {
            if (flag_plus) sign = '+';
            else if (flag_space) sign = ' ';
            uval = (unsigned long)val;
        }
        numlen = utoa_base(uval, numbuf, 10, 0);

        /* Apply precision (minimum digits) */
        int pzeros = 0;
        if (precision >= 0 && numlen < precision)
            pzeros = precision - numlen;

        int total = numlen + pzeros + (sign ? 1 : 0);
        int pad = (has_width && width > total) ? width - total : 0;
        char padchar = (flag_zero && !flag_minus && precision < 0) ? '0' : ' ';

        if (!flag_minus && padchar == ' ')
            out_pad(s, ' ', pad);
        if (sign)
            out_char(s, sign);
        if (!flag_minus && padchar == '0')
            out_pad(s, '0', pad);
        out_pad(s, '0', pzeros);
        out_string(s, numbuf);
        if (flag_minus)
            out_pad(s, ' ', pad);
        break;
    }

    case 'u':
    case 'x':
    case 'X':
    case 'o': {
        unsigned long uval;
        if (len_long)
            uval = va_arg(*pap, unsigned long);
        else
            uval = (unsigned long)va_arg(*pap, unsigned int);
        if (len_char)
            uval = (unsigned char)uval;
        else if (len_short)
            uval = (unsigned short)uval;

        int base = (spec == 'o') ? 8 : (spec == 'u') ? 10 : 16;
        int upper = (spec == 'X');
        numlen = utoa_base(uval, numbuf, base, upper);

        /* Determine prefix */
        const char *prefix = "";
        int preflen = 0;
        if (flag_hash && uval != 0) {
            if (spec == 'o') {
                /* Ensure at least one leading 0 */
                if (numbuf[0] != '0') {
                    prefix = "0";
                    preflen = 1;
                }
            } else if (spec == 'x') {
                prefix = "0x";
                preflen = 2;
            } else if (spec == 'X') {
                prefix = "0X";
                preflen = 2;
            }
        }
        /* Special case: %#o with value 0 should produce "0" */

        int pzeros = 0;
        if (precision >= 0 && numlen < precision)
            pzeros = precision - numlen;

        int total = preflen + pzeros + numlen;
        int pad = (has_width && width > total) ? width - total : 0;
        char padchar = (flag_zero && !flag_minus && precision < 0) ? '0' : ' ';

        if (!flag_minus && padchar == ' ')
            out_pad(s, ' ', pad);
        out_string(s, prefix);
        if (!flag_minus && padchar == '0')
            out_pad(s, '0', pad);
        out_pad(s, '0', pzeros);
        out_string(s, numbuf);
        if (flag_minus)
            out_pad(s, ' ', pad);
        break;
    }

    case 'c': {
        char c = (char)va_arg(*pap, int);
        int pad = (has_width && width > 1) ? width - 1 : 0;
        if (!flag_minus)
            out_pad(s, ' ', pad);
        out_char(s, c);
        if (flag_minus)
            out_pad(s, ' ', pad);
        break;
    }

    case 's': {
        const char *str = va_arg(*pap, const char *);
        if (!str) str = "(null)";

        /* Compute length respecting precision */
        int slen = 0;
        while (str[slen] && (precision < 0 || slen < precision))
            slen++;

        int pad = (has_width && width > slen) ? width - slen : 0;

        if (!flag_minus)
            out_pad(s, ' ', pad);
        for (int i = 0; i < slen; i++)
            out_char(s, str[i]);
        if (flag_minus)
            out_pad(s, ' ', pad);
        break;
    }

    case 'p': {
        unsigned long pval = (unsigned long)(unsigned int)va_arg(*pap, void *);
        out_string(s, "0x");
        numlen = utoa_base(pval, numbuf, 16, 0);
        out_string(s, numbuf);
        break;
    }

    case 'f': {
        /* Basic float formatting */
        double fval = va_arg(*pap, double);
        if (precision < 0) precision = 6;

        char sign = 0;
        if (fval < 0.0) {
            sign = '-';
            fval = -fval;
        } else if (flag_plus) {
            sign = '+';
        } else if (flag_space) {
            sign = ' ';
        }

        /* Integer part */
        unsigned long ipart = (unsigned long)fval;
        double frac = fval - (double)ipart;

        /* Build result */
        int intlen = utoa_base(ipart, numbuf, 10, 0);

        /* Compute total length */
        int total = intlen + (sign ? 1 : 0);
        if (precision > 0 || flag_hash)
            total += 1 + precision;  /* dot + digits */

        int pad = (has_width && width > total) ? width - total : 0;

        if (!flag_minus)
            out_pad(s, ' ', pad);
        if (sign)
            out_char(s, sign);
        out_string(s, numbuf);

        if (precision > 0 || flag_hash) {
            out_char(s, '.');
            for (int i = 0; i < precision; i++) {
                frac *= 10.0;
                int digit = (int)frac;
                out_char(s, '0' + digit);
                frac -= (double)digit;
            }
        }
        if (flag_minus)
            out_pad(s, ' ', pad);
        break;
    }

    case 'n': {
        int *np = va_arg(*pap, int *);
        if (np) *np = s->pos;
        break;
    }

    case '%':
        out_char(s, '%');
        break;

    default:
        /* Unknown specifier -- output as-is */
        out_char(s, '%');
        out_char(s, spec);
        break;
    }
}

static int mini_vprintf(out_state *s, const char *fmt, va_list ap) {
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '\0') break;
            format_arg(s, &fmt, (va_list *)&ap);
        } else {
            out_char(s, *fmt++);
        }
    }
    /* Null-terminate buffer if present */
    if (s->buf) {
        if (s->max < 0)
            s->buf[s->pos] = '\0';
        else if (s->pos < s->max)
            s->buf[s->pos] = '\0';
        else if (s->max > 0)
            s->buf[s->max - 1] = '\0';
    }
    return s->pos;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    out_state s = { buf, 0, -1 };
    int ret = mini_vprintf(&s, fmt, ap);
    va_end(ap);
    return ret;
}

int snprintf(char *buf, unsigned int size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    out_state s = { buf, 0, (int)size };
    int ret = mini_vprintf(&s, fmt, ap);
    va_end(ap);
    return ret;
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    out_state s = { (char *)0, 0, -1 };
    int ret = mini_vprintf(&s, fmt, ap);
    va_end(ap);
    return ret;
}

int fprintf(void *stream, const char *fmt, ...) {
    (void)stream;
    va_list ap;
    va_start(ap, fmt);
    out_state s = { (char *)0, 0, -1 };
    int ret = mini_vprintf(&s, fmt, ap);
    va_end(ap);
    return ret;
}

int vprintf(const char *fmt, va_list ap) {
    out_state s = { (char *)0, 0, -1 };
    return mini_vprintf(&s, fmt, ap);
}

int vsprintf(char *buf, const char *fmt, va_list ap) {
    out_state s = { buf, 0, -1 };
    return mini_vprintf(&s, fmt, ap);
}

int vsnprintf(char *buf, unsigned int size, const char *fmt, va_list ap) {
    out_state s = { buf, 0, (int)size };
    return mini_vprintf(&s, fmt, ap);
}

/* ================================================================
 * floor / floorf -- minimal implementation for torture tests.
 *
 * On i8085, double == float (both 32-bit IEEE 754).
 * floor(x) = largest integer <= x.
 *
 * Simple implementation using long conversion.
 * Valid for |x| < 2^31 (sufficient for torture tests).
 * ================================================================ */

double floor(double x) {
    if (x >= 0.0) {
        return (double)(long)x;
    } else {
        long i = (long)x;
        double d = (double)i;
        if (d == x)
            return x;
        return d - 1.0;
    }
}

float floorf(float x) {
    return (float)floor((double)x);
}
