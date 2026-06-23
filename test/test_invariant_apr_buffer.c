#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "buffer/apr_buffer.h"

START_TEST(test_apr_buffer_arraydup_bounds_check)
{
    // Invariant: apr_buffer_arraydup must not write beyond allocated destination buffer bounds
    // regardless of input size values
    
    // Payloads: exploit case (size causing overflow), boundary case (zero size), valid input
    struct {
        apr_size_t size;
        int zero_terminated;
        int nelts;
        const char *description;
    } test_cases[] = {
        {SIZE_MAX, 1, 2, "exploit: size + zero_terminated causes overflow"},
        {0, 0, 1, "boundary: zero size"},
        {1024, 0, 3, "valid: normal operation"}
    };
    
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < num_cases; i++) {
        // Create source buffer array
        apr_buffer_t *src_array = malloc(test_cases[i].nelts * sizeof(apr_buffer_t));
        ck_assert_ptr_nonnull(src_array);
        
        // Initialize source buffers with test data
        for (int j = 0; j < test_cases[i].nelts; j++) {
            src_array[j].size = test_cases[i].size;
            src_array[j].zero_terminated = test_cases[i].zero_terminated;
            
            // Allocate source memory if size > 0
            if (test_cases[i].size > 0) {
                src_array[j].d.mem = malloc(test_cases[i].size);
                ck_assert_ptr_nonnull(src_array[j].d.mem);
                memset(src_array[j].d.mem, 'A', test_cases[i].size);
            } else {
                src_array[j].d.mem = NULL;
            }
        }
        
        // Test the actual function
        apr_buffer_t *dst_array = NULL;
        apr_status_t result = apr_buffer_arraydup(&dst_array, src_array, 
                                                  (apr_buffer_alloc)malloc, NULL, 
                                                  test_cases[i].nelts);
        
        // Property: Function must either succeed with valid buffers or fail gracefully
        // without writing beyond allocated memory bounds
        if (result == APR_SUCCESS) {
            ck_assert_ptr_nonnull(dst_array);
            
            // Verify each destination buffer was properly allocated
            for (int j = 0; j < test_cases[i].nelts; j++) {
                if (test_cases[i].size + test_cases[i].zero_terminated > 0) {
                    ck_assert_ptr_nonnull(dst_array[j].d.mem);
                }
                ck_assert_uint_eq(dst_array[j].size, test_cases[i].size);
                ck_assert_int_eq(dst_array[j].zero_terminated, test_cases[i].zero_terminated);
            }
            
            // Clean up destination
            for (int j = 0; j < test_cases[i].nelts; j++) {
                if (dst_array[j].d.mem) {
                    free(dst_array[j].d.mem);
                }
            }
            free(dst_array);
        } else {
            // If function failed, ensure no partial writes corrupted memory
            ck_assert_ptr_null(dst_array);
        }
        
        // Clean up source
        for (int j = 0; j < test_cases[i].nelts; j++) {
            if (src_array[j].d.mem) {
                free(src_array[j].d.mem);
            }
        }
        free(src_array);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_apr_buffer_arraydup_bounds_check);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}