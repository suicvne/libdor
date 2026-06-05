#ifndef COMMON_H
#define COMMON_H

// IDK what you plan on doing with this. Just in case, here you go.
#if LIBDOR_NO_STDLIB_TYPES
    typedef unsigned long size_t;

    typedef unsigned long long uint64_t;
    typedef unsigned int uint32_t;
    typedef unsigned short uint16_t;
    typedef unsigned char uint8_t;

    typedef signed long long int64_t;
    typedef signed int int32_t;
    typedef signed short int16_t;
    typedef signed char int8_t;

    _Static_assert(sizeof(uint64_t) == 8);
    _Static_assert(sizeof(int64_t) == 8);

    _Static_assert(sizeof(uint32_t) == 4);
    _Static_assert(sizeof(int32_t) == 4);

    _Static_assert(sizeof(uint16_t) == 2);
    _Static_assert(sizeof(int16_t) == 2);

    _Static_assert(sizeof(uint8_t) == 1);
    _Static_assert(sizeof(int8_t) == 1);
#else
    #include <stddef.h> // size_t
    #include <stdint.h> // stdint
#endif

#endif // COMMON_H
