#define UUID_IMPLEMENTATION
#include "../uuid.h"

#include <stdio.h>

int main(int argc, char** argv)
{
    unsigned char uuid[UUID_SIZE];
    char uuidFormatted[UUID_FORMATTED_SIZE];
    size_t i;
    size_t count = 10;

    printf("uuid_generate_4()\n");
    {
        for (i = 0; i < count; i += 1) {
            uuid_generate_4(uuid);
            uuid_format(uuidFormatted, sizeof(uuidFormatted), uuid);
            printf("%s\n", uuidFormatted);
        }
    }
    printf("\n");

    printf("uuid_generate_4_ex()\n");
    {
        uuid_cryptorand rng;
        uuid_cryptorand_init(&rng);

        for (i = 0; i < count; i += 1) {
            uuid_generate_4_ex(uuid, &rng);
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
