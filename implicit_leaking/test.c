#include <sanitizer/dfsan_interface.h>

int main(void)
{
    int secret1 = 5;
    int secret2 = 8;

    int x = secret1;
    int y = secret2;

    int clean = 12;

    dfsan_set_label(
        1,
        &secret1,
        sizeof(secret1));

    dfsan_set_label(
        2,
        &secret2,
        sizeof(secret2));

    if (x + y + clean > 10) {
        clean = 1;
    }

    if (y + clean > 20) {
        clean = 2;
    }

    if (clean > 20) {
        clean = 3;
    }

    return clean;
}