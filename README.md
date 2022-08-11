# masterkey

masterkey is a CLI tool that can read data from physical address region, and
write data to physical address region.

### Features
- few runtime dependencies(glibc, libstdc++ only)
- stdin/stdout support

### Supported Platform
masterkey can work on Linux system that has `/dev/mem`, in other words,
Linux system that kernel is configured `CONFIG_DEVMEM=y` and `CONFIG_STRICT_DEVMEM=n`.

### How to Install

#### from Git repository

```bash
$ autoreconf
$ ./configure && make
$ sudo make install
```

### License
BSD 3-Clause License
