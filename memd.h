#ifndef __MEMD_H__
#define __MEMD_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/** 
 * Define USE_MEMD before including this file to enable MEMD functionality.
 * This allows MEMD to be easily enabled or disabled for different builds.
 */
#ifdef USE_MEMD

/** 
 * Maximum number of memory allocations MEMD will track.
 */
#define MEMD_MAX_ALLOCATIONS 1000

/** 
 * Maximum number of warnings MEMD will store.
 */
#define MEMD_MAX_WARNINGS 1000 

/** 
 * Macro to record a warning with contextual information.
 * Stores warnings up to MEMD_MAX_WARNINGS in MEMD_Data.warnings.
 */
#define WARN(msg, line, file) \
    if (MEMD_Data.warning_count < MEMD_MAX_WARNINGS) { \
        MEMD_Warning *warning = &MEMD_Data.warnings[MEMD_Data.warning_count++]; \
        snprintf(warning->message, sizeof(warning->message), "%s", msg); \
        warning->line = line; \
        warning->file = file; \
    }

/** 
 * Struct to represent a memory allocation event.
 */
typedef struct {
    size_t address; /**< The memory address allocated. */
    size_t size;    /**< The size of the allocation. */
    uint32_t line;  /**< The source line where the allocation occurred. */
    const char *file; /**< The source file where the allocation occurred. */
} MEMD_Mem;

/** 
 * Struct to represent a warning generated by MEMD.
 */
typedef struct {
    char message[128]; /**< Warning message. */
    uint32_t line;     /**< The source line where the warning was generated. */
    const char *file;  /**< The source file where the warning was generated. */
} MEMD_Warning;

/** 
 * Global structure to store tracking and warning data.
 */
struct {
    MEMD_Mem mem[MEMD_MAX_ALLOCATIONS]; /**< Array of tracked memory allocations. */
    size_t total_allocated_size; /**< Total size of all allocated memory. */
    size_t total_free_size; /**< Total size of all freed memory. */
    MEMD_Warning warnings[MEMD_MAX_WARNINGS]; /**< Array of generated warnings. */
    int warning_count; /**< Number of generated warnings. */
} MEMD_Data;

/** 
 * Returns a ptr to an allocated buffer wich contains the report of memory allocations and warnings.
 * Should be called before application exit to report potential memory leaks.
 * Don't forget to free it with 'memd_report_free' when not needed anymore, MEMD won't report it in this case ;)
 */
char* memd_report();

/**
 * Free a report created from MEMD.
 * This is important, since MEMD won't report the leak in this case.
*/
void memd_report_free(char* ptr);

#ifdef MEMD_IMPLEMENTATION

/** 
 * Thread-local flag to ignore memory tracking for specific operations.
 */
#if defined(_MSC_VER)
__declspec(thread) int _memd_ignore = 0;
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Thread_local int _memd_ignore = 0;
#else
__thread int _memd_ignore = 0;
#endif

/** 
 * Finds a tracked memory allocation by its address.
 * @return Pointer to the tracked memory allocation, or NULL if not found.
 */
MEMD_Mem *_find_by_address(size_t address) {
    for (uint32_t i = 0; i < MEMD_MAX_ALLOCATIONS; i++) {
        if (MEMD_Data.mem[i].address == address)
            return &MEMD_Data.mem[i];
    }

    return NULL;
}

/** 
 * Records a memory allocation.
 */
void _insert(size_t address, size_t size, uint32_t line, const char *file) {
    // check for null
    if (address == 0) {
        WARN("Memory allocation failed", line, file);
        return;
    }

    MEMD_Mem *mem = _find_by_address(0);
    // if the return value is null we need to increase the MEMD_MAX_ALLOCATIONS value
    if (mem == NULL) {
        WARN("Max allocations reached", line, file);
        return;
    }

    // save all the allocation info
    mem->address = address;
    mem->size = size;
    mem->line = line;
    mem->file = file;
    MEMD_Data.total_allocated_size += size;
}

/** 
 * Removes a tracked memory allocation, marking it as freed.
 * @return -1 on failure (e.g., double free detected), 0 on success.
 */
int _erase(size_t address, uint32_t line, const char *file) {
    if (address == 0) {
        WARN("Tried to free a null ptr", line, file);
        return -1;
    }

    MEMD_Mem *mem = _find_by_address(address);
    // if the address is not found we assume it is already deleted
    if (mem == NULL) {
        WARN("Double free detected", line, file);
        return -1;
    }

    // set address to null and update info
    mem->address = 0;
    MEMD_Data.total_free_size += mem->size;
    return 0;
}

/** 
 * Custom implementation of malloc for tracking purposes.
 */
void *_memd_malloc(size_t size, uint32_t line, const char *file) {
    void *ptr = malloc(size);

    if (_memd_ignore != 1) {
        // insert to memory data
        _insert((size_t)ptr, size, line, file);
    }

    return ptr;
}

/** 
 * Custom implementation of calloc for tracking purposes.
 * Allocates memory for an array of num elements of size bytes each and initializes all bytes to zero.
 */
void *_memd_calloc(size_t num, size_t size, uint32_t line, const char *file) {
    size_t totalSize = num * size;
    void *ptr = calloc(num, size);

    if (_memd_ignore != 1 && ptr != NULL) {
        _insert((size_t)ptr, totalSize, line, file);
    }

    return ptr;
}

/** 
 * Custom implementation of free for tracking purposes.
 */
void _memd_free(void *ptr, uint32_t line, const char *file) {
    if (_memd_ignore != 1) {
        // erase memory data
        if (_erase((size_t)ptr, line, file) == 0)
            free(ptr);
    }
}

/** 
 * Custom implementation of realloc for tracking purposes.
 * Changes the size of the memory block pointed to by ptr to size bytes.
 */
void *_memd_realloc(void *ptr, size_t size, uint32_t line, const char *file) {
    if (ptr == NULL) {
        // Equivalent to malloc
        return _memd_malloc(size, line, file);
    } else if (size == 0) {
        // Equivalent to free
        _memd_free(ptr, line, file);
        return NULL;
    } else {
        // Reallocate and update MEMD tracking if not ignored
        void *newPtr = realloc(ptr, size);
        if (newPtr != NULL && _memd_ignore != 1) {
            // Erase old entry
            _erase((size_t)ptr, line, file);
            // Insert new entry
            _insert((size_t)newPtr, size, line, file);
        }
        return newPtr;
    }
}

/** 
 * Pause memd memory tracking.
 */
static inline void memd_pause() {
    _memd_ignore = 1;
}

/** 
 * Resume paused memd memory tracking.
 */
static inline void memd_resume() {
    _memd_ignore = 0;
}

void memd_report_free(char* ptr) {
    free(ptr); // Free the memory allocated for the report.
}

char* memd_report() {
    size_t buffer_size = 1024 * 10; // Start with a 10KB buffer, adjust based on needs.
    char* report = (char*)malloc(buffer_size);
    if (!report) return NULL; // Failed to allocate memory for the report.

    size_t offset = 0; // Tracks the current offset in the buffer.

    // Helper macro to append formatted output to the buffer
    #define APPEND_TO_REPORT(fmt, ...) do { \
        int needed = snprintf(NULL, 0, fmt, ##__VA_ARGS__); \
        if (needed < 0) break; \
        if (offset + needed >= buffer_size) { \
            while (offset + needed >= buffer_size) buffer_size *= 2; \
            char* temp = (char*)realloc(report, buffer_size); \
            if (!temp) { \
                free(report); \
                return NULL; /* Handle reallocation failure */ \
            } \
            report = temp; \
        } \
        int written = snprintf(report + offset, buffer_size - offset, fmt, ##__VA_ARGS__); \
        if (written > 0) offset += (size_t)written; \
    } while (0)

    APPEND_TO_REPORT("\n----------------------------------\n");
    APPEND_TO_REPORT("MEMD Leak Summary:\n");
    APPEND_TO_REPORT("----------------------------------\n\n");
    APPEND_TO_REPORT("   Total Memory allocated %lu bytes\n", MEMD_Data.total_allocated_size);
    APPEND_TO_REPORT("   Total Memory freed     %lu bytes\n", MEMD_Data.total_free_size);
    APPEND_TO_REPORT("   Memory Leaked          %lu bytes\n", MEMD_Data.total_allocated_size - MEMD_Data.total_free_size);

    if (MEMD_Data.total_free_size != MEMD_Data.total_allocated_size) {
        APPEND_TO_REPORT("\n   Detailed Report:\n");
        for (int i = 0; i < MEMD_MAX_ALLOCATIONS; i++) {
            if (MEMD_Data.mem[i].address != 0) {
                APPEND_TO_REPORT("     Memory leak at %s:%d: (%lu bytes)\n", 
                    MEMD_Data.mem[i].file,
                    MEMD_Data.mem[i].line,
                    MEMD_Data.mem[i].size);
            }
        }
    }

    if (MEMD_Data.warning_count > 0) {
        APPEND_TO_REPORT("\n   Warnings:\n");
        for (int i = 0; i < MEMD_Data.warning_count; i++) {
            APPEND_TO_REPORT("    - %s:%d: %s\n", 
                MEMD_Data.warnings[i].file,
                MEMD_Data.warnings[i].line,
                MEMD_Data.warnings[i].message);
        }
    }

    APPEND_TO_REPORT("\n----------------------------------\n\n");

    #undef APPEND_TO_REPORT

    return report; // Return the dynamically allocated report buffer.
}


// Redefine standard allocation functions to use MEMD tracking versions.
#define malloc(size) _memd_malloc(size, __LINE__, __FILE__)
#define free(ptr) _memd_free(ptr, __LINE__, __FILE__)
#define calloc(num, size) _memd_calloc(num, size, __LINE__, __FILE__)
#define realloc(ptr, size) _memd_realloc(ptr, size, __LINE__, __FILE__)

#endif // MEMD_IMPLEMENTATION

#else // USE_MEMD not defined

/** 
 * Defines no-operation versions of MEMD functions when MEMD is disabled.
 */
#define memd_report() ((void)0)
#define memd_report_free(char* ptr) ((void)0)
#define memd_pause() ((void)0)
#define memd_resume() ((void)0)

#endif // USE_MEMD

#endif // __MEMD_H__
