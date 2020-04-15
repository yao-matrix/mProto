#include <cpuid.h>
#include <stdio.h>

static inline void getCpuid(unsigned int eaxIn, unsigned int data[4]) {
    __cpuid(eaxIn, data[0], data[1], data[2], data[3]);
}

static inline void getCpuidEx(unsigned int eaxIn, unsigned int ecxIn, unsigned int data[4]) {
    __cpuid_count(eaxIn, ecxIn, data[0], data[1], data[2], data[3]);
}

static unsigned int get32bitAsBE(const char *x) {
    return x[0] | (x[1] << 8) | (x[2] << 16) | (x[3] << 24);
}

bool has_mpx() {
    unsigned int data[4] = {};
    const unsigned int& EAX = data[0];
    const unsigned int& EBX = data[1];
    const unsigned int& ECX = data[2];
    const unsigned int& EDX = data[3];

    // check CPU
    getCpuid(0, data);
    const unsigned int maxNum = EAX;
    static const char intel[] = "ntel";
    if (ECX != get32bitAsBE(intel)) {
      // only check Intel CPU, for other CPUs return 0 by default
      return 0;    
    }

    if (maxNum >= 7) {
        getCpuidEx(7, 0, data);
        if (EBX & (1U << 14)) {
            return 1;
        }
    }

    return 0;
}


int main() {

  printf("MPX support status: %d \n", has_mpx());

  return 0;
}
