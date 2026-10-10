#include <stdlib.h>
#include <stdbool.h>
#include <sanitizer/dfsan_interface.h>

int main(int argc, char **argv) {
    if (argc != 4)
        return 1;

    int s1 = atoi(argv[1]);
    int s2 = atoi(argv[2]);
    int s3 = atoi(argv[3]);

    dfsan_set_label(1, &s1, sizeof(s1));
    dfsan_set_label(2, &s2, sizeof(s2));
    dfsan_set_label(4, &s3, sizeof(s3));

    int x = 0;
    int y = 0;
    int z = 0;

    int a = 10;
    int b = 10;
    int c = 10;
    int d = 10;

    if ((s1)) {
        x = 20;
        a=x;
        x = 0;
    } else if (s2) {
        y = 20;
        b = y;
        y = 0;
    } else if (s3) {
    } else {
    }

    // a = x;
    // b = y;

    // x = 10;
    // y = 10;

    c = a+b;

    return 0;
}
