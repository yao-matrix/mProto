#ifndef MPROTO_CPU_FEAT_H
#define MPROTO_CPU_FEAT_H

#include <cpuid.h>


/* refer to Intel Software Developer Manual for CPUID instruction details:
   https://software.intel.com/sites/default/files/managed/ad/01/253666-sdm-vol-2a.pdf
*/

static const Type tMPX = uint64(1) << 49;

static inline void getCpuid(unsigned int eaxIn, unsigned int data[4]) {
    __cpuid(eaxIn, data[0], data[1], data[2], data[3]);
}

static inline void getCpuidEx(unsigned int eaxIn, unsigned int ecxIn, unsigned int data[4]) {
    __cpuid_count(eaxIn, ecxIn, data[0], data[1], data[2], data[3]);
}

static unsigned int get32bitAsBE(const char *x) const {
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


#endif // MPROTO_CPU_FEAT_H
