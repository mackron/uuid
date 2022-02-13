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

    (void)argc;
    (void)argv;

    return 0;
}
