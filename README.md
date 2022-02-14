<h4 align="center">UUID Generator</h4>

<p align="center">
    <a href="https://discord.gg/9vpqbjU"><img src="https://img.shields.io/discord/712952679415939085?label=discord&logo=discord" alt="discord"></a>
    <a href="https://twitter.com/mackron"><img src="https://img.shields.io/twitter/follow/mackron?style=flat&label=twitter&color=1da1f2&logo=twitter" alt="twitter"></a>
</p>

This supports all UUID versions defined in RFC 4122 except version 2. For version 3 and 5 you will
need to provide your own MD5 and SHA-1 hashing implementation by defining the some macros before
the implementation of this library. Below is an example:

    #define UUID_MD5_CTX_TYPE               md5_context
    #define UUID_MD5_INIT(ctx)              md5_init(ctx)
    #define UUID_MD5_FINAL(ctx, digest)     md5_finalize(ctx, (unsigned char*)(digest))
    #define UUID_MD5_UPDATE(ctx, src, sz)   md5_update(ctx, src, (size_t)(sz))

    #define UUID_SHA1_CTX_TYPE              sha1_context
    #define UUID_SHA1_INIT(ctx)             sha1_init(ctx)
    #define UUID_SHA1_FINAL(ctx, digest)    sha1_finalize(ctx, (unsigned char*)(digest))
    #define UUID_SHA1_UPDATE(ctx, src, sz)  sha1_update(ctx, src, (size_t)(sz));

A public domain MD5 and SHA-1 hashing implmentation can be found here:

    * https://github.com/mackron/md5
    * https://github.com/mackron/sha1

These are included as submodules in this repository for your convenience but need not be used if
you would rather use a different implementation.

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