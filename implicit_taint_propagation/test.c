#include <stdio.h>
#include <stdbool.h>
#include <sanitizer/dfsan_interface.h>

int main(void)
{
    int secret1 = 1;
    int secret2 = 0;

    dfsan_set_label(
        1,
        &secret1,
        sizeof(secret1));

    dfsan_set_label(
        2,
        &secret2,
        sizeof(secret2));

    int x = 0;
    int y = 0;
    int z = 0;
    int a = 0;
    int b = 0;
    int c = 0;

    /*
     * secret1 controls the outer branch.
     *
     * secret2 controls the nested condition.
     *
     * Only stores belonging to branches actually entered
     * during this execution receive implicit taint.
     */
    if (secret1) {

        x = 10;

        if (secret2) {
            y = 20;
        } else {
            z = 30;
        }

    } else if (secret2) {
        x = 10;
        y = 40;

    } else {
        x = 10;
        z = 50;
    }

    a = x;
    b = y;
    c = z;

    printf(
        "secret1 = %u\n"
        "secret2 = %u\n"
        "x       = %u\n"
        "y       = %u\n"
        "z       = %u\n"
        "a       = %u\n"
        "b       = %u\n"
        "c       = %u\n",

        (unsigned)dfsan_read_label(
            &secret1,
            sizeof(secret1)),

        (unsigned)dfsan_read_label(
            &secret2,
            sizeof(secret2)),

        (unsigned)dfsan_read_label(
            &x,
            sizeof(x)),

        (unsigned)dfsan_read_label(
            &y,
            sizeof(y)),

        (unsigned)dfsan_read_label(
            &z,
            sizeof(z)),

        (unsigned)dfsan_read_label(
            &a,
            sizeof(a)),

        (unsigned)dfsan_read_label(
            &b,
            sizeof(b)),

        (unsigned)dfsan_read_label(
            &c,
            sizeof(c))
    );

    return 0;
}