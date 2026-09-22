#include <stdio.h>
#include <sanitizer/dfsan_interface.h>

int main()
{
    int secret1 = 5;
    int secret2 = 8;
    int secret3 = 12;

    dfsan_set_label(1, &secret1, sizeof(secret1));
    dfsan_set_label(2, &secret2, sizeof(secret2));
    // dfsan_set_label(4, &secret3, sizeof(secret3));

    int x = secret1;

    if (x > 10) {
        printf("Condition 1: true\n");
    }

    int y = secret2;
    int z = secret3;

    if (y + z > 20) {
        printf("Condition 2: true\n");
    }

    if (z > 20) {
        printf("Z is clean\n");
    }

    int a = secret1 + 5;
    int b = secret2 * 2;

    if (a > 10 && b > 10) {
        printf("Condition 3: true\n");
    }

    int c = secret1 + secret2;

    if ((c * 2) > 25) {
        printf("Condition 4: true\n");
    }

    int p = secret1;
    int q = secret2;
    int r = secret3;

    if (p + q + r > 30) {
        printf("Condition 5: true\n");
    }

    if (secret1 > 2) {
        if (secret2 > 5) {
            if (secret3 > 10) {
                printf("Nested condition: all true\n");
            }
        }
    }

    if (secret1 > 100 || secret2 > 100) {
        printf("Condition 7: true\n");
    }

    if (secret1 > secret2) {
        printf("Condition 8: true\n");
    }

    return 0;
}