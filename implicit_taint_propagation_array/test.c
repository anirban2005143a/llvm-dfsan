#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/dfsan_interface.h>

int main(int argc, char **argv) {
    if (argc != 7)
        return 1;

    /* Inputs: a b c secret1 secret2 secret3 */
    int a  = atoi(argv[1]);
    int b  = atoi(argv[2]);
    int c  = atoi(argv[3]);
    int secret1 = atoi(argv[4]);
    int secret2 = atoi(argv[5]);
    int secret3 = atoi(argv[6]);

    /* Apply DFSan labels to secret inputs */
    dfsan_set_label(1, &secret1, sizeof(secret1));
    dfsan_set_label(2, &secret2, sizeof(secret2));
    dfsan_set_label(4, &secret3, sizeof(secret3));

    int values[4] = {a, secret1, 0, 42};
    int x = 0;
    int y = 0;
    int z = 0;
    int cnt = 0;

    /* Taint tagging dependent on public input 'a' */
    if (a > 0) {
        /* values[2] becomes tainted with secret2's label */
        dfsan_set_label(dfsan_read_label(&secret2, sizeof(secret2)), &values[2], sizeof(values[2]));
    }
    /* When a <= 0, values[2] remains untainted (label 0) */

    /* Secret input secret1 paths are active ONLY if public input b > 0 */
    if (b > 0) {
        if (secret1 > 0) {
            x = 20;
        } else if (secret1 < 0) {
            x = 10;
        } else {
            x = 5;
        }

        /* Secret loop capped at 2 iterations */
        for (int i = 0; i < secret1 && i < 2; ++i) {
            ++cnt;
            values[3]++;
        }
    }

    /* Secret input secret2 paths */
    if (secret2 != 0) {
        y = 2;
    } else {
        y = 1;
    }

    /* Secret input secret3 paths are active ONLY if public input c > 0 */
    if (c > 0) {
        if (secret3 > 0) {
            z += 5;
        } else if (secret3 < 0) {
            z -= 5;
        } else {
            z += 1;
        }
    }

    return 0;
}