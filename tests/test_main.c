#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Test function declarations
extern void test_log_entry(void);
extern void test_queue(void);
extern void test_config(void);

int main(void) {
    printf("Running Log Aggregator Tests...\n\n");
    
    printf("Testing log_entry...\n");
    test_log_entry();
    printf("✓ log_entry tests passed\n\n");
    
    printf("Testing queue...\n");
    test_queue();
    printf("✓ queue tests passed\n\n");
    
    printf("Testing config...\n");
    test_config();
    printf("✓ config tests passed\n\n");
    
    printf("All tests passed!\n");
    return 0;
}