#include <stdio.h>
#include <stdbool.h>
#include <sanitizer/dfsan_interface.h>

static void print_label(
    const char *name,
    const void *p,
    size_t n)
{
    printf(
        "%s = %u\n",
        name,
        (unsigned)dfsan_read_label(p, n));
}

int main(void)
{
    int secret1 = 1;
    int secret2 = 0;
    int secret3 = 0;

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

    int x = 0;
    int y = 0;
    int z = 0;

    int a = 0;
    int b = 0;
    int c = 0;

    int same = 0;

    if (secret1) {
        x = 10;
    } else if (secret2) {
        y = 40;
    } else if (secret3) {
        z = 40;
    } else {
        z = 50;
    }

    if (secret1) {
        same = 7;
    } else {
        same = 7;
    }

    a = x;
    b = y;
    c = z;

    print_label(
        "secret1",
        &secret1,
        sizeof(secret1));

    print_label(
        "secret2",
        &secret2,
        sizeof(secret2));

    print_label(
        "x",
        &x,
        sizeof(x));

    print_label(
        "y",
        &y,
        sizeof(y));

    print_label(
        "z",
        &z,
        sizeof(z));

    print_label(
        "a",
        &a,
        sizeof(a));

    print_label(
        "b",
        &b,
        sizeof(b));

    print_label(
        "c",
        &c,
        sizeof(c));

    print_label(
        "same",
        &same,
        sizeof(same));

    return 0;
}