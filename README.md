# User_level_loader
This is a loader for Linux ELF file.



### Programs

`apager`: ELF loader with all-at-once loading

`dpager`: ELF loader with demand loading

`apager2`: Similar to `apager`, but execution returns to the loader

`backtoback`: ELF loader with back-to-back loading *(multiple programs)*

`uthread`: ELF loader with user-level threads *(multiple programs)*



### Tests

`hello_world`: Simple ELF file without libc. Original source code comes from https://cirosantilli.com/elf-hello-world#code-3.

`test1`, `test2`: Simple test program compiled with glibc. These programs call `malloc()`

`test3`: Simple test program compiled with glibc. This program calls `malloc()` and `strcpy()`

`segfault`: Simple test program compiled with glibc. This program tries to access 0x00, so execution should fail.

`thread1`, `thread2`: Simple test program compiled with glibc. They call `thread_yield()` function from `user_library.h`, so they can be used to test `uthread`.



### Memory offset

| Program        | Offset     |
| -------------- | ---------- |
| `hello_world`  | 0x20000000 |
| `thread1`      | 0x20000000 |
| `thread2`      | 0x40000000 |
| other programs | 0x20000000 |

