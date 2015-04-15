# read-bad-flash

This little tool can help recover a file from a faulty device.

It will try to extract data by reading the input file in chunks of given size even if there are some types of hardware read errors.

When the tool encounters a broken chunk it will ask the user whether they want to retry reading or create an empty (zero-filled) chunk instead.

## Buidling

Use `premake5` to generate a GNU Makefile, then compile using GNU Make:


```
$ premake5 --os=linux --cc=gcc gmake2
$ make config=release
```

Build tested using GCC and Arch Linux.

It should be possible to use other operating systems and compilers, please let me know if you succeed or fail to do so.

## Project Homepage

https://github.com/0xebef/read-bad-flash

## License and Copyright

License: GPLv3 or later

Copyright (c) 2015, 0xebef
