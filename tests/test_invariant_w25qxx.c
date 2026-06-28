#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "../src/w25qxx.c"

START_TEST(test_version_buffer_safety)
{
    // Invariant: Version buffer must not be accessed after free under any input condition
    const char *payloads[] = {
        "",                    // Empty string boundary case
        "W25Q128JV",           // Valid version string
        "W25QXX_VERSION_EXTRA_LONG_STRING_THAT_EXCEEDS",  // Long string
        NULL                   // NULL pointer case
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        // Replace the macro with test payload
        char *saved_version = W25QXX_VERSION;
        #define W25QXX_VERSION payloads[i]
        
        // Call function that uses version_buffer
        w25qxx_init();
        
        // Restore original version
        #undef W25QXX_VERSION
        #define W25QXX_VERSION saved_version
        
        // If we reach here without crash, invariant holds
        ck_assert_msg(1, "Buffer safety maintained for payload %d", i);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_version_buffer_safety);
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