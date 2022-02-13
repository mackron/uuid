/*
UUID generation. Choice of public domain or MIT-0. See license statements at the end of this file.

David Reid - mackron@gmail.com
*/

/*
This supports all UUID versions defined in RFC 4122 except version 2. For version 3 and 5 you will
need to provide your own MD5 and SHA-1 hasher by defining the some macros before the implementation
of this library. Below is an example:

    #define UUID_MD5_CTX_TYPE               struct md5_context
    #define UUID_MD5_INIT(ctx)              md5_init(ctx)
    #define UUID_MD5_FINAL(ctx, digest)     md5_finalize(ctx, (struct md5_digest*)(digest))
    #define UUID_MD5_UPDATE(ctx, src, sz)   md5_update(ctx, src, (uint32_t)(sz))

    #define UUID_SHA1_CTX_TYPE              SHA1_CTX
    #define UUID_SHA1_INIT(ctx)             SHA1Init(ctx)
    #define UUID_SHA1_FINAL(ctx, digest)    SHA1Final(digest, ctx)
    #define UUID_SHA1_UPDATE(ctx, src, sz)  SHA1Update(ctx, src, (uint32_t)(sz));

There is no need to link to anything with this library. You can use UUID_IMPLEMENTATION to define
the implementation section, or you can use uuid.c if you prefer a traditional header/source pair.

Use the following APIs to generate a UUID:

    uuid1(unsigned char* pUUID, uuid_rand* pRNG);
    uuid3(unsigned char* pUUID, const unsigned char* pNamespaceUUID, const char* pName);
    uuid4(unsigned char* pUUID, uuid_rand* pRNG);
    uuid5(unsigned char* pUUID, const unsigned char* pNamespaceUUID, const char* pName);
    uuid_ordered(unsigned char* pUUID, uuid_rand* pRNG);

If you want to use a time-based ordered UUID you can use `uuid_ordered()`. Note that this is not
officially allowed by RFC 4122. This does not encode a version as it would break ordering.

Use the following APIs to format the UUID as a string:

    uuid_format(char* pDst, size_t dstCap, const unsigned char* pUUID);

The size of the UUID buffer must be at least `UUID_SIZE` (16 bytes). For formatted strings the
destination buffer should be at least `UUID_SIZE_FORMATTED`.

Example:

    ```c
    unsigned char uuid[UUID_SIZE];
    uuid4(uuid, NULL);

    char str[UUID_SIZE_FORMATTED];
    uuid_format(str, sizeof(str), uuid);
    ```

With the code above the default random number generator will be used (second parameter). If you
want to use your own random number generator, you can pass in a random number generator:

    ```c
    uuid4(uuid, &myRNG);
    ```

The default random number generator is cryptorand: https://github.com/mackron/cryptorand. If you
have access to the implementation section, you can make use of this easily:

    ```c
    uuid_cryptorand rng;
    uuid_cryprorand_init(&rng);

    for (i = 0; i < count; i += 1) {
        uuid4(uuid, &rng);

        // ... do something with the UUID ...
    }

    uuiid_cryptorand_uninit(&rng);
    ```

Alternatively you can implement your own random number generator by inheritting from
`uuid_rand_callbacks` like so:

    ```c
    typedef struct
    {
        uuid_rand_callbacks cb;
    } my_rng;

    static uuid_result my_rng_generate(uuid_rand* pRNG, void* pBufferOut, size_t byteCount)
    {
        my_rng* pMyRNG = (my_rng*)pRNG;

        // ... do random number generation here. Output byteCount bytes to pBufferOut.

        return UUID_SUCCESS;
    }

    ...

    // Initialize your random number generator.
    my_rng myRNG;
    rng.cb.onGenerator = my_rng_generate;

    // Generate your GUID.
    uuid4(uuid, &myRNG);
    ```

You can disable cryptorand and compile time with `UUID_NO_CRYPTORAND`, but by doing so you will be
required to specify your own random number generator. This is useful if you already have a good
quality random number generator in your code base and want to save a little bit of space.
*/
#ifndef uuid_h
#define uuid_h

#ifdef __cplusplus
extern "C" {
#endif

/*
Need to set this or else we'll get errors about timespec not being defined. This also needs to be
set before including any standard headers.
*/
#if !defined(_WIN32)
    #ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE   700
    #else
        #if _XOPEN_SOURCE < 500
        #error _XOPEN_SOURCE must be >= 500. uuid is not usable.
        #endif
    #endif
#endif

#include <stddef.h>

#define UUID_SIZE           16
#define UUID_SIZE_FORMATTED 37

typedef enum
{
    UUID_SUCCESS           =  0,
    UUID_ERROR             = -1,
    UUID_INVALID_ARGS      = -2,
    UUID_INVALID_OPERATION = -3,
    UUID_NOT_IMPLEMENTED   = -29
} uuid_result;

typedef void uuid_rand;
typedef struct
{
    uuid_result (* onGenerate)(uuid_rand* pRNG, void* pBufferOut, size_t byteCount);
} uuid_rand_callbacks;

uuid_result uuid_rand_generate(uuid_rand* pRNG, void* pBufferOut, size_t byteCount);


/* Generation. */
uuid_result uuid1(unsigned char* pUUID, uuid_rand* pRNG);
uuid_result uuid3(unsigned char* pUUID, const unsigned char* pNamespaceUUID, const char* pName);
uuid_result uuid4(unsigned char* pUUID, uuid_rand* pRNG);
uuid_result uuid5(unsigned char* pUUID, const unsigned char* pNamespaceUUID, const char* pName);
uuid_result uuid_ordered(unsigned char* pUUID, uuid_rand* pRNG);

/* Formatting. */
uuid_result uuid_format(char* dst, size_t dstCap, const unsigned char* pUUID);

#ifdef __cplusplus
}
#endif
#endif  /* uuid_h */

#if defined(UUID_IMPLEMENTATION)
#ifndef uuid_c
#define uuid_c

typedef unsigned short     uuid_uint16;
typedef unsigned int       uuid_uint32;
typedef unsigned long long uuid_uint64;

#include <string.h>
#define UUID_COPY_MEMORY(dst, src, sz)  memcpy((dst), (src), (sz))
#define UUID_ZERO_MEMORY(p, sz)         memset((p), 0, (sz))
#define UUID_ZERO_OBJECT(o)             UUID_ZERO_MEMORY((o), sizeof(*o))

#ifndef UUID_ASSERT
    #include <assert.h>
    #define UUID_ASSERT(condition)  assert(condition)
#endif

#include <time.h>   /* For timespec. */

#ifndef TIME_UTC
#define TIME_UTC    1
#endif

#if (defined(_MSC_VER) && _MSC_VER < 1900) || defined(__DMC__)  /* 1900 = Visual Studio 2015 */
struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};
#endif


uuid_result uuid_rand_generate(uuid_rand* pRNG, void* pBufferOut, size_t byteCount)
{
    uuid_rand_callbacks* pCallbacks = (uuid_rand*)pRNG;

    if (pBufferOut == NULL) {
        return UUID_INVALID_ARGS;
    }

    UUID_ZERO_MEMORY(pBufferOut, byteCount);

    if (pCallbacks == NULL || pCallbacks->onGenerate == NULL) {
        return UUID_INVALID_ARGS;
    }

    return pCallbacks->onGenerate(pRNG, pBufferOut, byteCount);
}


#if !defined(UUID_NO_CRYPTORAND)
#define CRYPTORAND_IMPLEMENTATION
#include "external/cryptorand/cryptorand.h"

static uuid_result uuid_result_from_cryptorand(cryptorand_result result)
{
    return (uuid_result)result;
}

typedef struct
{
    uuid_rand_callbacks base;
    cryptorand rng;
} uuid_cryptorand;

static uuid_result uuid_cryptorand_generate(uuid_rand* pRNG, void* pBufferOut, size_t byteCount)
{
    uuid_result result;

    if (pRNG == NULL) {
        return UUID_INVALID_ARGS;
    }

    result = uuid_result_from_cryptorand(cryptorand_generate(&((uuid_cryptorand*)pRNG)->rng, pBufferOut, byteCount));
    if (result != UUID_SUCCESS) {
        return result;
    }

    return UUID_SUCCESS;
}

static uuid_result uuid_cryptorand_init(uuid_cryptorand* pRNG)
{
    uuid_result result;

    if (pRNG == NULL) {
        return UUID_INVALID_ARGS;
    }

    UUID_ZERO_OBJECT(pRNG);

    result = uuid_result_from_cryptorand(cryptorand_init(&pRNG->rng));
    if (result != UUID_SUCCESS) {
        return result;  /* Failed to initialize RNG. */
    }

    pRNG->base.onGenerate = uuid_cryptorand_generate;

    return UUID_SUCCESS;
}

static void uuid_cryptorand_uninit(uuid_cryptorand* pRNG)
{
    if (pRNG == NULL) {
        return;
    }

    cryptorand_uninit(&pRNG->rng);
    UUID_ZERO_OBJECT(pRNG);
}
#endif /* UUID_NO_CRYPTORAND */


#if defined(_WIN32)
#include <windows.h>

static int uuid_timespec_get(struct timespec* ts, int base)
{
    FILETIME ft;
    LONGLONG current100Nanoseconds;

    if (ts == NULL) {
        return 0;   /* 0 = error. */
    }

    ts->tv_sec  = 0;
    ts->tv_nsec = 0;

    /* Currently only supporting UTC. */
    if (base != TIME_UTC) {
        return 0;   /* 0 = error. */
    }

    GetSystemTimeAsFileTime(&ft);
    current100Nanoseconds = (((LONGLONG)ft.dwHighDateTime << 32) | (LONGLONG)ft.dwLowDateTime);
    current100Nanoseconds = current100Nanoseconds - ((LONGLONG)116444736 * 1000000000); /* Windows to Unix epoch. Normal value is 116444736000000000LL, but VC6 doesn't like 64-bit constants. */

    ts->tv_sec  = (time_t)(current100Nanoseconds / 10000000);
    ts->tv_nsec =  (long)((current100Nanoseconds - (ts->tv_sec * 10000000)) * 100);

    return base;
}
#else
#include <sys/time.h>   /* For timeval. */

struct timespec uuid_timespec_from_timeval(struct timeval* tv)
{
    struct timespec ts;

    ts.tv_sec  = tv->tv_sec;
    ts.tv_nsec = tv->tv_usec * 1000;

    return ts;
}

static int uuid_timespec_get(struct timespec* ts, int base)
{
    /*
    This is annoying to get working on all compilers. Here's the hierarchy:

        * If using C11, use timespec_get(); else
        * If _POSIX_C_SOURCE >= 199309L, use clock_gettime(CLOCK_REALTIME, ...); else
        * Fall back to gettimeofday().
    */
    #if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__APPLE__)
    {
        return timespec_get(ts, base);
    }
    #else
    {
        if (base != TIME_UTC) {
            return 0;   /* Only TIME_UTC is supported. 0 = error. */
        }

        #if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199309L
        {
            if (clock_gettime(CLOCK_REALTIME, ts) != 0) {
                return 0;   /* Failed to retrieve the time. 0 = error. */
            }

            /* Getting here means we were successful. On success, need to return base (strange...) */
            return base;
        }
        #else
        {
            struct timeval tv;
            if (gettimeofday(&tv, NULL) != 0) {
                return 0;   /* Failed to retrieve the time. 0 = error. */
            }

            *ts = uuid_timespec_from_timeval(&tv);
            return base;
        }
        #endif  /* _POSIX_C_SOURCE >= 199309L */
    }
    #endif  /* C11 */
}
#endif

static uuid_result uuid_get_time(uuid_uint64* pTime)
{
    struct timespec ts;

    if (pTime == NULL) {
        return UUID_INVALID_ARGS;
    }

    *pTime = 0;

    if (uuid_timespec_get(&ts, TIME_UTC) == 0) {
        return UUID_ERROR;  /* Failed to retrieve time. */
    }

    *pTime  = (ts.tv_sec * 10000000) + (ts.tv_nsec / 100);      /* In 100-nanoseconds resolution. */
    *pTime += (((uuid_uint64)0x01B21DD2 << 32) | 0x13814000);   /* Conversion from Unix Epoch to UUID Epoch. Weird format here is for compatibility with VC6 because it doesn't like 64-bit constants. */

    return UUID_SUCCESS;
}

static uuid_result uuid1_internal(unsigned char* pUUID, uuid_rand* pRNG)
{
    uuid_result result;
    uuid_uint64 time;
    uuid_uint32 timeLow;
    uuid_uint16 timeMid;
    uuid_uint16 timeHiAndVersion;

    UUID_ASSERT(pUUID != NULL);
    UUID_ASSERT(pRNG  != NULL);

    result = uuid_get_time(&time);
    if (result != UUID_SUCCESS) {
        return result;
    }

    timeLow          = (uuid_uint32)((time >>  0) & 0xFFFFFFFF);
    timeMid          = (uuid_uint16)((time >> 32) & 0x0000FFFF);
    timeHiAndVersion = (uuid_uint16)((time >> 48) & 0x00000FFF) | 0x1000;

    /* Time Low */
    pUUID[0] = (unsigned char)((timeLow >> 24) & 0xFF);
    pUUID[1] = (unsigned char)((timeLow >> 16) & 0xFF);
    pUUID[2] = (unsigned char)((timeLow >>  8) & 0xFF);
    pUUID[3] = (unsigned char)((timeLow >>  0) & 0xFF);

    /* Time Mid */
    pUUID[4] = (unsigned char)((timeMid >> 8) & 0xFF);
    pUUID[5] = (unsigned char)((timeMid >> 0) & 0xFF);

    /* Time High and Version */
    pUUID[6] = (unsigned char)((timeHiAndVersion >> 8) & 0xFF);
    pUUID[7] = (unsigned char)((timeHiAndVersion >> 0) & 0xFF);

    /* For the clock sequence and node ID we're always using a random number. */
    result = uuid_rand_generate(pRNG, pUUID + 8, UUID_SIZE - 8);
    if (result != UUID_SUCCESS) {
        UUID_ZERO_MEMORY(pUUID, UUID_SIZE);
        return result;
    }

    /* Byte 8 needs to be updated to reflect the variant. In our case it'll always be Variant 1. */
    pUUID[8] = 0x80 | (pUUID[8] & 0x3F);

    return UUID_SUCCESS;
}

static uuid_result uuid3_internal(unsigned char* pUUID, const unsigned char* pNamespaceUUID, const char* pName)
{
#if defined(UUID_MD5_CTX_TYPE)
    unsigned char hash[20];
    UUID_MD5_CTX_TYPE ctx;

    UUID_MD5_INIT(&ctx);
    {
        UUID_MD5_UPDATE(&ctx, pNamespaceUUID, UUID_SIZE);
        UUID_MD5_UPDATE(&ctx, (const unsigned char*)pName, strlen(pName));
    }
    UUID_MD5_FINAL(&ctx, hash);

    UUID_COPY_MEMORY(pUUID, hash, UUID_SIZE);

    /* Byte 6 needs to be updated so the version number is set appropriately. */
    pUUID[6] = 0x30 | (pUUID[6] & 0x0F);

    /* Byte 8 needs to be updated to reflect the variant. In our case it'll always be Variant 1. */
    pUUID[8] = 0x80 | (pUUID[8] & 0x3F);

    return UUID_SUCCESS;
#else
    (void)pUUID;
    (void)pNamespaceUUID;
    (void)pName;
    return UUID_NOT_IMPLEMENTED;
#endif
}

static uuid_result uuid4_internal(unsigned char* pUUID, uuid_rand* pRNG)
{
    uuid_result result;

    UUID_ASSERT(pUUID != NULL);
    UUID_ASSERT(pRNG  != NULL);

    /* First just generate some random numbers. */
    result = uuid_rand_generate(pRNG, pUUID, UUID_SIZE);
    if (result != UUID_SUCCESS) {
        UUID_ZERO_MEMORY(pUUID, UUID_SIZE);
        return result;
    }

    /* Byte 6 needs to be updated so the version number is set appropriately. */
    pUUID[6] = 0x40 | (pUUID[6] & 0x0F);

    /* Byte 8 needs to be updated to reflect the variant. In our case it'll always be Variant 1. */
    pUUID[8] = 0x80 | (pUUID[8] & 0x3F);

    return UUID_SUCCESS;
}

static uuid_result uuid5_internal(unsigned char* pUUID, const unsigned char* pNamespaceUUID, const char* pName)
{
#if defined(UUID_SHA1_CTX_TYPE)
    unsigned char hash[20];
    UUID_SHA1_CTX_TYPE ctx;

    UUID_SHA1_INIT(&ctx);
    {
        UUID_SHA1_UPDATE(&ctx, pNamespaceUUID, UUID_SIZE);
        UUID_SHA1_UPDATE(&ctx, (const unsigned char*)pName, strlen(pName));
    }
    UUID_SHA1_FINAL(&ctx, hash);

    UUID_COPY_MEMORY(pUUID, hash, UUID_SIZE);

    /* Byte 6 needs to be updated so the version number is set appropriately. */
    pUUID[6] = 0x50 | (pUUID[6] & 0x0F);

    /* Byte 8 needs to be updated to reflect the variant. In our case it'll always be Variant 1. */
    pUUID[8] = 0x80 | (pUUID[8] & 0x3F);

    return UUID_SUCCESS;
#else
    (void)pUUID;
    (void)pNamespaceUUID;
    (void)pName;
    return UUID_NOT_IMPLEMENTED;
#endif
}

uuid_result uuid_ordered_internal(unsigned char* pUUID, uuid_rand* pRNG)
{
    uuid_result result;
    uuid_uint64 time;
    uuid_uint32 timeLow;
    uuid_uint16 timeMid;
    uuid_uint16 timeHi;

    UUID_ASSERT(pUUID != NULL);
    UUID_ASSERT(pRNG  != NULL);

    result = uuid_get_time(&time);
    if (result != UUID_SUCCESS) {
        return result;
    }

    timeLow = (uuid_uint32)((time >>  0) & 0xFFFFFFFF);
    timeMid = (uuid_uint16)((time >> 32) & 0x0000FFFF);
    timeHi  = (uuid_uint16)((time >> 48) & 0x00000FFF);

    /* Time High and Version */
    pUUID[0] = (unsigned char)((timeHi  >>  8) & 0xFF);
    pUUID[1] = (unsigned char)((timeHi  >>  0) & 0xFF);

    /* Time Mid */
    pUUID[2] = (unsigned char)((timeMid >>  8) & 0xFF);
    pUUID[3] = (unsigned char)((timeMid >>  0) & 0xFF);

    /* Time Low */
    pUUID[4] = (unsigned char)((timeLow >> 24) & 0xFF);
    pUUID[5] = (unsigned char)((timeLow >> 16) & 0xFF);
    pUUID[6] = (unsigned char)((timeLow >>  8) & 0xFF);
    pUUID[7] = (unsigned char)((timeLow >>  0) & 0xFF);


    /* For the clock sequence and node ID we're always using a random number. */
    result = uuid_rand_generate(pRNG, pUUID + 8, UUID_SIZE - 8);
    if (result != UUID_SUCCESS) {
        UUID_ZERO_MEMORY(pUUID, UUID_SIZE);
        return result;
    }

    /* Setting the version number breaks the ordering property of these UUIDs so I'm leaving this unset. */
    /*pUUID[6] = 0x40 | (pUUID[6] & 0x0F);*/

    /* Byte 8 needs to be updated to reflect the variant. In our case it'll always be Variant 1. */
    pUUID[8] = 0x80 | (pUUID[8] & 0x3F);

    return UUID_SUCCESS;
}


typedef enum
{
    UUID_VERSION_1       = 1,   /* Timed. */
    UUID_VERSION_2       = 2,   /* ??? */
    UUID_VERSION_3       = 3,   /* Named with MD5 hashing. */
    UUID_VERSION_4       = 4,   /* Random. */
    UUID_VERSION_5       = 5,   /* Named with SHA1 hashing. */
    UUID_VERSION_ORDERED = 100  /* Unofficial. Similar to version 1, but the time part is swapped so that it's sorted by time. Useful for database keys. */
} uuid_version;

static uuid_result uuidn(unsigned char* pUUID, uuid_rand* pRNG, const unsigned char* pNamespaceUUID, const char* pName, uuid_version version)
{
    uuid_result result;
#if !defined(UUID_NO_CRYPTORAND)
    uuid_cryptorand cryptorandRNG;
#endif

    if (pUUID == NULL) {
        return UUID_INVALID_ARGS;
    }

    UUID_ZERO_MEMORY(pUUID, UUID_SIZE);

    /* Some versions need s random number generator. */
    if (version == UUID_VERSION_1 || version == UUID_VERSION_4 || version == UUID_VERSION_ORDERED) {
        if (pRNG == NULL) {
        #if !defined(UUID_NO_CRYPTORAND)
            result = uuid_cryptorand_init(&cryptorandRNG);
            if (result != UUID_SUCCESS) {
                return result;
            }

            pRNG = &cryptorandRNG;
        #else
            return UUID_INVALID_ARGS;   /* No random number generator available. */
        #endif
        }
    }

    switch (version)
    {
        case UUID_VERSION_1:       result = uuid1_internal(pUUID, pRNG);                  break;
        case UUID_VERSION_2:       result = UUID_NOT_IMPLEMENTED;                         break;
        case UUID_VERSION_3:       result = uuid3_internal(pUUID, pNamespaceUUID, pName); break;
        case UUID_VERSION_4:       result = uuid4_internal(pUUID, pRNG);                  break;
        case UUID_VERSION_5:       result = uuid5_internal(pUUID, pNamespaceUUID, pName); break;
        case UUID_VERSION_ORDERED: result = uuid_ordered_internal(pUUID, pRNG);           break;
        default:                   result = UUID_INVALID_ARGS;                            break;  /* Unknown or unsupported version. */
    };

#if !defined(UUID_NO_CRYPTORAND)
    if (pRNG == &cryptorandRNG) {
        uuid_cryptorand_uninit(&cryptorandRNG);
    }
#endif

    return result;
}


uuid_result uuid1(unsigned char* pUUID, uuid_rand* pRNG)
{
    return uuidn(pUUID, pRNG, NULL, NULL, UUID_VERSION_1);
}

uuid_result uuid3(unsigned char* pUUID, const unsigned char* pNamespaceUUID, const char* pName)
{
    return uuidn(pUUID, NULL, pNamespaceUUID, pName, UUID_VERSION_3);
}

uuid_result uuid4(unsigned char* pUUID, uuid_rand* pRNG)
{
    return uuidn(pUUID, pRNG, NULL, NULL, UUID_VERSION_4);
}

uuid_result uuid5(unsigned char* pUUID, const unsigned char* pNamespaceUUID, const char* pName)
{
    return uuidn(pUUID, NULL, pNamespaceUUID, pName, UUID_VERSION_5);
}

uuid_result uuid_ordered(unsigned char* pUUID, uuid_rand* pRNG)
{
    return uuidn(pUUID, pRNG, NULL, NULL, UUID_VERSION_ORDERED);
}




static void uuid_format_byte(char* dst, unsigned char byte)
{
    const char* hex = "0123456789abcdef";
    dst[0] = hex[(byte & 0xF0) >> 4];
    dst[1] = hex[(byte & 0x0F)     ];
}

uuid_result uuid_format(char* dst, size_t dstCap, const unsigned char* pUUID)
{
    const char* format = "xxxx-xx-xx-xx-xxxxxx";

    if (dst == NULL) {
        return UUID_INVALID_ARGS;
    }

    if (dstCap < UUID_SIZE_FORMATTED) {
        if (dstCap > 0) {
            dst[0] = '\0';
        }

        return UUID_INVALID_ARGS;
    }

    if (pUUID == NULL) {
        dst[0] = '\0';
        return UUID_INVALID_ARGS;
    }

    /* All we need to do here is convert the string to hex with dashes. */
    for (;;) {
        if (format[0] == '\0') {
            break;
        }

        if (format[0] == 'x') {
            uuid_format_byte(dst, pUUID[0]);
            dst   += 2;
            pUUID += 1;
        } else {
            dst[0] = format[0];
            dst += 1;
        }

        format += 1;
    }

    /* Never forget to null terminate. */
    dst[0] = '\0';

    return UUID_SUCCESS;
}

#endif  /* uuid_c */
#endif  /* UUID_IMPLEMENTATION */

/*
This software is available as a choice of the following licenses. Choose
whichever you prefer.

===============================================================================
ALTERNATIVE 1 - Public Domain (www.unlicense.org)
===============================================================================
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

===============================================================================
ALTERNATIVE 2 - MIT No Attribution
===============================================================================
Copyright 2022 David Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
