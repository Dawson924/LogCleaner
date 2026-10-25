# Log Cleaner

A small Windows tool for finding and deleting `.log` files.

## Build

Requires CMake and a C++ compiler.

```
cmake -B build -S .
cmake --build build --config Release
```

The executable will be at `build/LogCleaner.exe`.

## Settings

`LogCleaner.ini` is created next to the executable.

## Notes

- The program only looks for `.log` files in its own directory and subdirectories