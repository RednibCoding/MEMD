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

#include <stdio.h>

int main() {
    printf("Hello, World");
    return 0;
}