#include <stdlib.h>

int abs(int num) {
    if (num < 0) {
        return -num; // Flip sign if negative
    }
    return num; // Return as-is if positive or zero
}