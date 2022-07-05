#ifndef PIT_COMMON_H
#define PIT_COMMON_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

// got it from here
// https://stackoverflow.com/questions/3982348/implement-generic-swap-macro-in-c
#define swap(x,y) do \ 
   { unsigned char swap_temp[sizeof(x) == sizeof(y) ? (signed)sizeof(x) : -1]; \
     memcpy(swap_temp,&y,sizeof(x)); \
     memcpy(&y,&x,       sizeof(x)); \
     memcpy(&x,swap_temp,sizeof(x)); \
    } while(0)

// and that is from here
// https://stackoverflow.com/questions/3437404/min-and-max-in-c
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

typedef struct {
    uint8_t r, g, b, a;
} col32;

static inline col32 Col32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (col32) { .r = r, .g = g, .b = b, .a = a };
}

#endif // PIT_COMMON_H