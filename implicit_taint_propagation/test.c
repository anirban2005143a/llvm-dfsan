#include <stdlib.h>
#include <sanitizer/dfsan_interface.h>

int main(int argc, char **argv) {
    if (argc != 4)
        return 1;

    int secret1 = atoi(argv[1]);
    int secret2 = atoi(argv[2]);
    int secret3 = atoi(argv[3]);

    dfsan_set_label(1, &secret1, sizeof(secret1));
    dfsan_set_label(2, &secret2, sizeof(secret2));
    dfsan_set_label(4, &secret3, sizeof(secret3));

    int p = 10;
    int q = 0;
    int r = 0;

    int a = 10;
    int b = 20;
    int c = 10;
    int d = 10;

    if (secret1) {
        p = (a+b);
        a=p;
        p = 10;
    } else if (secret2) {
        p = (c+b) + 10;
        b=p;
        p = 10;
    } else if (secret3) {
        p = (2*b)-a;
        c=p;
        p = 20;
    } else {
        p = a+c+10;
        d=p;
        p = 10;
    }

    return 0;
}
