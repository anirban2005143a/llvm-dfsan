// FILE: test.c

#include <stdio.h>
#include <stdlib.h>

#include <sanitizer/dfsan_interface.h>

int main(int argc, char **argv)
{
    if (argc != 4)
        return 1;

    int secret1 = atoi(argv[1]);
    int secret2 = atoi(argv[2]);
    int secret3 = atoi(argv[3]);

    dfsan_set_label(
        1,
        &secret1,
        sizeof(secret1));

    dfsan_set_label(
        2,
        &secret2,
        sizeof(secret2));

    dfsan_set_label(
        4,
        &secret3,
        sizeof(secret3));

    int a = 10;
    int b = 21;
    int c = 5;

    int x = 0;
    int y = 0;
    int z = 0;

    if (secret1) {

        x = a + b - 3;

    } else if (secret2) {

        x = a + b;
        y = 20;

    } else if (secret3) {

        x = (2 * b) - a;

    } else {

        x = c + b;
        z = 29;
    }

    printf(
        "x=%d y=%d z=%d "
        "x_label=%u "
        "y_label=%u "
        "z_label=%u\n",
        x,
        y,
        z,
        (unsigned)dfsan_read_label(
            &x,
            sizeof(x)),
        (unsigned)dfsan_read_label(
            &y,
            sizeof(y)),
        (unsigned)dfsan_read_label(
            &z,
            sizeof(z)));

    return 0;
}