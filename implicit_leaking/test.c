#include <stdio.h>
#include <sanitizer/dfsan_interface.h>

int main()
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

    if (x + y + z > 10) {
        printf("Condition 1: true\n");
    }

    if (y + z > 20) {
        printf("Condition 2: true\n");
    }

    if (z > 20) {
        printf("Z is clean\n");
    }

    return 0;
}