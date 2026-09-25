#include <sanitizer/dfsan_interface.h>

int main(void)
{
    int secret1 = 5;
    int secret2 = 8;

    dfsan_set_label(
        1,
        &secret1,
        sizeof(secret1));

    dfsan_set_label(
        2,
        &secret2,
        sizeof(secret2));

    int x = secret1;
    int y = secret2;
    int z = 12;

    volatile int sink = 0;

    if (x + y + z > 10) {
        sink = 1;
    }

    if (y + z > 20) {
        sink = 2;
    }

    if (z > 20) {
        sink = 3;
    }

    return sink;
}