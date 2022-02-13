#include "../external/md5.c"
#include "../external/sha1.c"

#define UUID_MD5_CTX_TYPE               struct md5_context
#define UUID_MD5_INIT(ctx)              md5_init(ctx)
#define UUID_MD5_FINAL(ctx, digest)     md5_finalize(ctx, (struct md5_digest*)(digest))
#define UUID_MD5_UPDATE(ctx, src, sz)   md5_update(ctx, src, (uint32_t)(sz))

#define UUID_SHA1_CTX_TYPE              SHA1_CTX
#define UUID_SHA1_INIT(ctx)             SHA1Init(ctx)
#define UUID_SHA1_FINAL(ctx, digest)    SHA1Final(digest, ctx)
#define UUID_SHA1_UPDATE(ctx, src, sz)  SHA1Update(ctx, src, (uint32_t)(sz));

#define UUID_IMPLEMENTATION
#include "../uuid.h"

#include <stdio.h>

/* These functions are taken straight from RFC 4122. Only used for testing. */
#if defined(_WIN32)
void get_system_time_rfc4122(uuid_uint64* uuid_time)
{
    ULARGE_INTEGER time;

    /* NT keeps time in FILETIME format which is 100ns ticks since
       Jan 1, 1601. UUIDs use time in 100ns ticks since Oct 15, 1582.
       The difference is 17 Days in Oct + 30 (Nov) + 31 (Dec)
       + 18 years and 5 leap days. */
    GetSystemTimeAsFileTime((FILETIME *)&time);
    time.QuadPart +=

          (unsigned __int64) (1000*1000*10)       // seconds
        * (unsigned __int64) (60 * 60 * 24)       // days
        * (unsigned __int64) (17+30+31+365*18+5); // # of days
    *uuid_time = time.QuadPart;
}
#else
void get_system_time_rfc4122(uuid_uint64* uuid_time)
{
    struct timeval tp;

    gettimeofday(&tp, (struct timezone *)0);

    /* Offset between UUID formatted times and Unix formatted times.
       UUID UTC base time is October 15, 1582.
       Unix base time is January 1, 1970.*/
    *uuid_time = ((uuid_uint64)tp.tv_sec * 10000000)
        + ((uuid_uint64)tp.tv_usec * 10)
        + 0x01B21DD213814000ULL;
}
#endif



int main(int argc, char** argv)
{
    unsigned char uuid[UUID_SIZE];
    char uuidFormatted[UUID_FORMATTED_SIZE];
    size_t i;
    size_t count = 10;

    printf("Timing:\n");
    {
        uuid_uint64 time_rfc4122;
        uuid_uint64 time_uuid;

        get_system_time_rfc4122(&time_rfc4122);
        printf("RFC 4122: %llu\n", time_rfc4122);

        uuid_get_time(&time_uuid);
        printf("UUID:     %llu\n", time_uuid);
    }
    printf("\n");
    

    printf("uuid1()\n");
    {
        for (i = 0; i < count; i += 1) {
            uuid1(uuid);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");

    printf("uuid1_ex()\n");
    {
        uuid_cryptorand rng;
        uuid_cryptorand_init(&rng);

        for (i = 0; i < count; i += 1) {
            uuid1_ex(uuid, &rng);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }

        uuid_cryptorand_uninit(&rng);
    }
    printf("\n");


    printf("uuid4()\n");
    {
        for (i = 0; i < count; i += 1) {
            uuid4(uuid);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");

    printf("uuid4_ex()\n");
    {
        uuid_cryptorand rng;
        uuid_cryptorand_init(&rng);

        for (i = 0; i < count; i += 1) {
            uuid4_ex(uuid, &rng);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }

        uuid_cryptorand_uninit(&rng);
    }
    printf("\n");


    printf("uuid_ordered()\n");
    {
        for (i = 0; i < count; i += 1) {
            uuid_ordered(uuid);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");

    printf("uuid_ordered_ex()\n");
    {
        uuid_cryptorand rng;
        uuid_cryptorand_init(&rng);

        for (i = 0; i < count; i += 1) {
            uuid_ordered_ex(uuid, &rng);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }

        uuid_cryptorand_uninit(&rng);
    }
    printf("\n");


    printf("uuid3()\n");
    {
        for (i = 0; i < count; i += 1) {
            unsigned char ns[] = {0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}; /* "6ba7b811-9dad-11d1-80b4-00c04fd430c8" */
            //unsigned char ns[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* "00000000-0000-0000-0000-000000000000" */

            uuid3(uuid, ns, "Hello, World!");
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");


    printf("uuid5()\n");
    {
        for (i = 0; i < count; i += 1) {
            unsigned char ns[] = {0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}; /* "6ba7b811-9dad-11d1-80b4-00c04fd430c8" */
            //unsigned char ns[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* "00000000-0000-0000-0000-000000000000" */

            uuid5(uuid, ns, "Hello, World!");
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");

    (void)argc;
    (void)argv;

    return 0;
}
