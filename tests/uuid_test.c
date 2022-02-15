#include "../external/md5/md5.c"    /* <-- Enables version 3. */
#include "../external/sha1/sha1.c"  /* <-- Enables version 5. */

/*
The macros below can be used to enable a custom MD5 and/or SHA-1 implementation.
*/
#if 0
#define UUID_MD5_CTX_TYPE               md5_context
#define UUID_MD5_INIT(ctx)              md5_init(ctx)
#define UUID_MD5_FINAL(ctx, digest)     md5_finalize(ctx, (unsigned char*)(digest))
#define UUID_MD5_UPDATE(ctx, src, sz)   md5_update(ctx, src, (size_t)(sz))

#define UUID_SHA1_CTX_TYPE              sha1_context
#define UUID_SHA1_INIT(ctx)             sha1_init(ctx)
#define UUID_SHA1_FINAL(ctx, digest)    sha1_finalize(ctx, (unsigned char*)(digest))
#define UUID_SHA1_UPDATE(ctx, src, sz)  sha1_update(ctx, src, (size_t)(sz));
#endif

#define UUID_IMPLEMENTATION
#include "../uuid.h"

#include <stdio.h>

int main(int argc, char** argv)
{
    unsigned char uuid[UUID_SIZE];
    char uuidFormatted[UUID_SIZE_FORMATTED];
    size_t i;
    size_t count = 10;


    printf("uuid1()\n");
    {
        for (i = 0; i < count; i += 1) {
            uuid1(uuid, NULL);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");


    printf("uuid3()\n");
    {
        for (i = 0; i < count; i += 1) {
            unsigned char ns[] = {0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}; /* "6ba7b811-9dad-11d1-80b4-00c04fd430c8" */
            /*unsigned char ns[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};*/ /* "00000000-0000-0000-0000-000000000000" */

            uuid3(uuid, ns, "Hello, World!");
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");


    printf("uuid4()\n");
    {
        for (i = 0; i < count; i += 1) {
            uuid4(uuid, NULL);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");


    printf("uuid5()\n");
    {
        for (i = 0; i < count; i += 1) {
            unsigned char ns[] = {0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}; /* "6ba7b811-9dad-11d1-80b4-00c04fd430c8" */
            /*unsigned char ns[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};*/ /* "00000000-0000-0000-0000-000000000000" */

            uuid5(uuid, ns, "Hello, World!");
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");


    printf("uuid_ordered()\n");
    {
        for (i = 0; i < count; i += 1) {
            uuid_ordered(uuid, NULL);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");


    (void)argc;
    (void)argv;

    return 0;
}
