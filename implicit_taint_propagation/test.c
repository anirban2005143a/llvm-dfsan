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

    int p = 0;
    int q = 0;
    int r = 0;

    int a = 10;
    int b = 20;
    int c = 10;
    int d = 10;

    if ((s1 && s2)) {
        p = (a+b);
        a=p;
        p = 10;
    } else if (s2) {
        // p = (c+b) + 10;
        // b=p;
        // p = 10;
    } else if (s3) {
        // p = (2*b)-a;
        // c=p;
        // p = 20;
    } else {
        // p = a+c+10;
        // d=p;
        // p = 10;
    }

    return 0;
}
