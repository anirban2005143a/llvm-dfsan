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

    int x = 0;
    int y = 0;
    int z = 0;

    int a = 10;
    int b = 20;
    int c = 10;

    if (secret1) {
        x = 30;
        a = x;
        x = 10;
    } else if (secret2) {
        x = 40;
        b = x;
        x = 10;
        // y = 20;
    } else if (secret3) {
        x = 50;
        c = x;
        x = 10;
        z = 30;
    } else {
        x = 10;
        // z = 100;
    }

    return 0;
}
