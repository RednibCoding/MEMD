# MEMD: (Mem)ory Leak (D)etection Library

MEMD is a drop-in, single-header library designed for easy integration into C
projects to facilitate memory leak detection. By defining `MEMD_IMPLEMENTATION`
before including `memd.h`, the implementation is also included.

## Activation

To activate MEMD in your project, define `USE_MEMD`. This signals that MEMD's
functionality is enabled, making all memory allocations and deallocations
trackable.

```c
#define USE_MEMD
#include "memd.h"
```

If `USE_MEMD` is not defined, calls to `memd_report` and `memd_ignore` will be
replaced by empty macros, eliminating the need to remove these calls manually
from your code.

## Usage

After including MEMD in your project, it operates transparently, tracking memory
allocations and deallocations. To generate a report of memory usage and
potential leaks, call `memd_report` before your application exits:

```c
int main() {
    // Your code here
    memd_report(); // Print the memory leak report
    return 0;
}
```

### Ignoring Memory Operations

Use `memd_ignore` to temporarily disable tracking for specific memory
operations. This is particularly useful for handling memory allocated or freed
by external libraries, which MEMD cannot track directly.

```c
memd_ignore(1); // Ignore the following memory operation(s)
free(external_ptr); // Free operation by an external library, not to be tracked
memd_ignore(0); // Resume tracking memory operations
```

This feature ensures that MEMD's tracking does not interfere with or falsely
report on memory managed outside of its purview, such as libraries that allocate
or free memory internally.

## Integration

MEMD is designed to be minimally invasive and easily removable. Its drop-in
nature allows for quick integration into any C project, providing immediate
insights into memory management and potential leaks. When you're done debugging,
simply remove or comment out the `#define USE_MEMD` directive to disable MEMD's
tracking capabilities without needing to cleanse your code of MEMD-related
calls.

## Features

- **Memory Leak Detection**: Automatically detects and reports memory leaks in
  your application.
- **Debugging Support**: Provides detailed information about each leaked memory
  block, including size, location (file and line number), and more.
- **Selective Tracking**: Allows selective enabling/disabling of memory tracking
  to accommodate external library calls.
- **Warnings**: Captures and reports potential issues, such as double frees or
  attempts to free unallocated memory.

## Requirements

- C Compiler: Compatible with any standard C compiler (GCC, Clang, MSVC).
- Standard Library: Uses standard C library functions for memory management and
  output.

## Example

Below is a simple example demonstrating how to integrate and use MEMD in a C
project:

```c
#define USE_MEMD // Activate memd (must be first)
#define MEMD_IMPLEMENTATION // Also include implementation (must be second)
#include "memd.h" // include memd.h (must be last)

#include <string.h>

void i_will_leak() {
    char* data = (char*)malloc(200); // Tracked by MEMD (this will leak and be reported by MEMD)
}

int main() {
    char* my_data = (char*)malloc(100); // Tracked by MEMD
    strcpy(my_data, "Hello, MEMD!");
    printf("%s\n", my_data);
    
    free(my_data); // tracked by MEMD
    free(my_data); // tracked by MEMD (Double free will be reported by MEMD)

    memd_pause();
    free(my_data); // this will be ignored by MEMD, so it wont be reported
    memd_resume();

    // Call the bad function
    i_will_leak();

    // Get the MEMD report
    char* report = memd_report();
    if (report) {
        printf("%s", report); // Use the report.
        memd_report_free(report); // Free the report when done.
    }
    
    return 0;
}
```

The output of MEMD will be:

```
----------------------------------
MEMD Leak Summary:
----------------------------------

   Total Memory allocated 300 bytes
   Total Memory freed     100 bytes
   Memory Leaked          200 bytes

   Detailed Report:
     Memory leak at main.c:8: (200 bytes)

   Warnings:
    - main.c:17: Double free detected

----------------------------------
```

By following the above guidelines, you can effectively utilize MEMD to identify
and resolve memory leaks in your C projects, enhancing code quality and
reliability.

> Note: MEMD is not able to detect nor will it report any "`use-after-free`"
> errors. This would involve lexical and semantical analyses, which MEMD does
> not do (yet?)
