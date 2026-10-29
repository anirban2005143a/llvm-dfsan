#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/dfsan_interface.h>


int main(int argc, char **argv) {
    if (argc != 7)
        return 1;

    /* Positional arguments matching: a b c s1 s2 s3 */
    int a  = atoi(argv[1]);
    int b  = atoi(argv[2]);
    int c  = atoi(argv[3]);
    int s1 = atoi(argv[4]);
    int s2 = atoi(argv[5]);
    int s3 = atoi(argv[6]);

    /* Assign DataFlowSanitizer label bitmasks to secret inputs */
    dfsan_set_label(1, &s1, sizeof(s1));
    dfsan_set_label(2, &s2, sizeof(s2));
    dfsan_set_label(4, &s3, sizeof(s3));

    int x = 0;
    int y = 0;

    /* Public input 'b' controls whether secret input 's1' is checked */
    if (b > 0) {
        if (s1 > 0) {
            x = 20;
        } else {
            x = 10;
        }
    } else {
        x = 5; /* s1 is bypassed completely when b <= 0 */
    }

    /* Secret input 's2' branch */
    if (s2 != 0) {
        y = 2;
    } else {
        y = 1;
    }

    /* Secret loop dependent on s1 (capped) */
    int cnt = 0;
    for (int i = 0; i < s1 && i < 2; ++i) {
        ++cnt;
    }


    return 0;
}