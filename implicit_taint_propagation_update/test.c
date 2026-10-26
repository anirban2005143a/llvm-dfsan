#include <stdlib.h>
#include <sanitizer/dfsan_interface.h>

/* Scalar globals are included in the end-of-main debug report. */
int global_count = 0;
int global_result = 0;
float global_ratio = 0.0f;

/* Exercise a function, conditionals, and a loop. */
static int helper(int public_input, int secret_input) {
    int result = public_input;

    if (secret_input > 0) {
        result += 5;
    } else if (secret_input < 0) {
        result -= 5;
    } else {
        result += 1;
    }

    int iterations = 0;
    for (int j = 0; j < public_input && j < 4; ++j) {
        ++iterations;
    }

    global_count += iterations;
    global_result += result + iterations;
    return result + iterations;
}

int main(int argc, char **argv) {
    if (argc != 7)
        return 1;

    /* Input order is documented by the header in test_cases.txt. */
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int c = atoi(argv[3]);
    int s1 = atoi(argv[4]);
    int s2 = atoi(argv[5]);
    int s3 = atoi(argv[6]);

    /* The analyzer detects these source-level labels automatically. */
    dfsan_set_label(1, &s1, sizeof(s1));
    dfsan_set_label(2, &s2, sizeof(s2));
    dfsan_set_label(4, &s3, sizeof(s3));

    int x = 0;
    int y = 0;
    int z = 0;
    int cnt = 0;
    int helper_result = 0;

    if (s1 != 0) {
        x = 20;
    } else if (s2 != 0) {
        x = 10;
    } else {
        x = 0;
    }

    if (s2 != 0) {
        y = 2;
    } else if (s3 != 0) {
        y = 3;
    } else {
        y = 0;
    }

    /* Secret-dependent loop iteration count; iteration count is capped. */
    for (int i = 0; i < s1 && i < 4; ++i) {
        ++cnt;
    }

    helper_result = helper(a, s3);

    if (b > 0) {
        z = helper_result + c;
    } else if (c > 0) {
        z = helper_result - c;
    } else {
        z = helper_result;
    }

    /* Exercise an implicit-flow-sensitive scalar and floating-point values. */
    float ratio = (float)(a + cnt) / 2.0f;
    if (x != 0)
        ratio += 0.5f;
    global_ratio = ratio;
    global_result += (s2 != 0) ? y : x;

    return 0;
}
