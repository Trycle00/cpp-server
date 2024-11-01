#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#define PRINT_TEST(format, argv...)                           \
    {                                                         \
        char* dyn_buf     = nullptr;                          \
        const int written = asprintf(&dyn_buf, format, argv); \
        if (written != -1)                                    \
        {                                                     \
            printf("PRINT_TEST: %s\n", dyn_buf);              \
            free(dyn_buf);                                    \
        }                                                     \
    }

void test(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char* dyn_buf;

    printf("Demo asprintf:\n");
    const int written_1 = asprintf(&dyn_buf, fmt, args);
    va_end(args);
    printf("dyn_buf: %s; %i chars were written\n", dyn_buf, written_1);
    free(dyn_buf);

    printf("Demo vasprintf:\n");
    // va_list args;
    va_start(args, fmt);
    const int written_2 = vasprintf(&dyn_buf, fmt, args);
    va_end(args);
    printf("dyn_buf: \"%s\"; %i chars were written\n", dyn_buf, written_2);
    free(dyn_buf);
}

int main(void)
{
    printf("======================================\n");

    test("Testing... %d, %d, %d", 1, 2, 3);

    printf("--------------------------------------\n");

    PRINT_TEST("Testing... %d, %d, %d", 1, 2, 3);

    printf("--------------------------------------\n");
}