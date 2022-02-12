<h4 align="center">UUID Generator</h4>

<p align="center">
    <a href="https://discord.gg/9vpqbjU"><img src="https://img.shields.io/discord/712952679415939085?label=discord&logo=discord" alt="discord"></a>
    <a href="https://twitter.com/mackron"><img src="https://img.shields.io/twitter/follow/mackron?style=flat&label=twitter&color=1da1f2&logo=twitter" alt="twitter"></a>
</p>

Currently only version 4 is supported, but support for other versions might be added later.

Use `uuid_generate_4()` to generate a UUID. Use `uuid_generate_4_ex()` if you want to use a custom
random number generator instead of the default (see below). If you want to format the UUID for
presentation purposes, use `uuid_format()` after generating the UUID.

By default this uses crytorand for random number generation: https://github.com/mackron/cryptorand.
If you would rather use your own, you can do so by implementing `uuid_rand_callbacks` and passing a
pointer to it to `uuid_generate_4_ex()`. You can use `UUID_NO_CRYPTORAND` to disable cryptorand at
compile time.