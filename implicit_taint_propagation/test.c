#include <stdio.h>
#include <sanitizer/dfsan_interface.h>

int main(void)
{
    int secret1 = 1;
    int secret2 = 1;

    dfsan_set_label(1, &secret1, sizeof(secret1));
    dfsan_set_label(2, &secret2, sizeof(secret2));

    int x = 0, y = 0, z = 0, a = 0, b = 0, c = 0;

    if(secret1 + secret2){
        x = 10;
    } else if(secret1){
        y = 10;
    } else {
        z = 10;
    }

    a = x;
    b = y;
    c = z;

    printf(
        "secret1   label = %u\n"
        "secret2   label = %u\n"
        "x         label = %u\n"
        "y         label = %u\n"
        "z         label = %u\n"
        "a         label = %u\n"
        "b         label = %u\n"
        "c         label = %u\n",
        // dfsan_get_label(secret1),
        // dfsan_get_label(secret2),
        // dfsan_get_label(x),
        // dfsan_get_label(y),
        // dfsan_get_label(z),
        // dfsan_get_label(a),
        // dfsan_get_label(b),
        // dfsan_get_label(c)

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