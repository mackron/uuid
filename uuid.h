/*
UUID generation. Choice of public domain or MIT-0. See license statements at the end of this file.

David Reid - mackron@gmail.com
*/

/*
Currently only version 4 is supported, but support for other versions might be added later.

Use `uuid_generate_4()` to generate a UUID. Use `uuid_generate_4_ex()` if you want to use a custom
random number generator instead of the default (see below). If you want to format the UUID for
presentation purposes, use `uuid_format()` after generating the UUID.

By default this uses crytorand for random number generation: https://github.com/mackron/cryptorand.
If you would rather use your own, you can do so by implementing `uuid_rand_callbacks` and passing a
pointer to it to `uuid_generate_4_ex()`. You can use `UUID_NO_CRYPTORAND` to disable cryptorand at
compile time.
*/
#ifndef uuid_h
#define uuid_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define UUID_SIZE           16
#define UUID_FORMATTED_SIZE 37

typedef enum
{
    UUID_SUCCESS      =  0,
    UUID_ERROR        = -1,
    UUID_INVALID_ARGS = -2
} uuid_result;

typedef void uuid_rand;
typedef struct
{
    uuid_result (* onGenerate)(uuid_rand* pRNG, void* pBufferOut, size_t byteCount);
} uuid_rand_callbacks;

/*
Generates the raw data of a UUID.
*/
uuid_result uuid_generate_4_ex(unsigned char* pUUID, uuid_rand* pRNG);

/*
Generates the raw data of a UUID.
*/
uuid_result uuid_generate_4(unsigned char* pUUID);

/*
Formats the UUID data as a string.
*/
uuid_result uuid_format(char* pBufferOut, size_t bufferOutCap, const unsigned char* pUUID);

#ifdef __cplusplus
}
#endif
#endif  /* uuid_h */

#if defined(UUID_IMPLEMENTATION)
#ifndef uuid_c
#define uuid_c

#include <string.h>
#define UUID_ZERO_MEMORY(p, sz) memset((p), 0, (sz))
#define UUID_ZERO_OBJECT(o)     UUID_ZERO_MEMORY((o), sizeof(*o))

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


uuid_result uuid_generate_4_ex(unsigned char* pUUID, uuid_rand* pRNG)
{
    uuid_result result;

    if (pUUID == NULL) {
        return UUID_INVALID_ARGS;
    }

    UUID_ZERO_MEMORY(pUUID, UUID_SIZE);

    if (pRNG == NULL) {
        return UUID_INVALID_ARGS;
    }

    /* First just generate some random numbers. */
    result = ((uuid_rand_callbacks*)pRNG)->onGenerate(pRNG, pUUID, UUID_SIZE);
    if (result != UUID_SUCCESS) {
        UUID_ZERO_MEMORY(pUUID, UUID_SIZE);
        return result;
    }

    /* Byte 6 needs to be updated so the version number is set appropriately. We're using version 4 for now. */
    pUUID[6] = 0x40 | (pUUID[7] & 0x0F);

    /* Byte 8 needs to be updated to reflect the variant. In our case it'll always be Variant 1. */
    pUUID[8] = 0x80 | (pUUID[9] & 0x3F);

    return UUID_SUCCESS;
}

uuid_result uuid_generate_4(unsigned char* pUUID)
{
    if (pUUID == NULL) {
        return UUID_INVALID_ARGS;
    }

    UUID_ZERO_MEMORY(pUUID, UUID_SIZE);

#if !defined(UUID_NO_CRYPTORAND)
    uuid_result result;
    uuid_cryptorand rng;

    result = uuid_cryptorand_init(&rng);
    if (result != UUID_SUCCESS) {
        return result;
    }

    result = uuid_generate_4_ex(pUUID, &rng);

    /* We're done with the random number generator. */
    uuid_cryptorand_uninit(&rng);

    return result;
#else
    /* cryptorand has been disabled. No way to generate random numbers. */
    return UUID_ERROR;
#endif
}

static void uuid_format_byte(char* pBufferOut, unsigned char byte)
{
    const char* pHexChars = "0123456789abcdef";
    pBufferOut[0] = pHexChars[(byte & 0xF0) >> 4];
    pBufferOut[1] = pHexChars[(byte & 0x0F)     ];
}

uuid_result uuid_format(char* pBufferOut, size_t bufferOutCap, const unsigned char* pUUID)
{
    const char* format = "xxxx-xx-xx-xx-xxxxxx";

    if (pBufferOut == NULL) {
        return UUID_INVALID_ARGS;
    }

    pBufferOut[0] = '\0';

    if (bufferOutCap < UUID_FORMATTED_SIZE || pUUID == NULL) {
        return UUID_INVALID_ARGS;
    }

    /* All we need to do here is convert the string to hex with dashes. */
    for (;;) {
        if (format[0] == '\0') {
            break;
        }

        if (format[0] == 'x') {
            uuid_format_byte(pBufferOut, pUUID[0]);
            pBufferOut += 2;
            pUUID      += 1;
        } else {
            pBufferOut[0] = format[0];
            pBufferOut += 1;
        }

        format += 1;
    }

    /* Never forget to null terminate. */
    pBufferOut[0] = '\0';

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
