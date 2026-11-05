/*
 * Small, standalone KLEE demonstration program.
 *
 * Input contract (all decimal signed 32-bit integers):
 *   argv[1]    mode
 *   argv[2]    left
 *   argv[3]    right
 *   argv[4..7] data[0..3]
 *
 * The program expects exactly 7 user arguments (argc == 8).
 * The array has a compile-time fixed size. Loops have fixed bounds.
 */
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE 4
#define REQUIRED_ARGC (1 + 3 + ARRAY_SIZE)

static int classify(int value)
{
    if (value < 0)
        return -1;
    if (value == 0)
        return 0;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != REQUIRED_ARGC)
        return 90;

    int mode = atoi(argv[1]);
    int left = atoi(argv[2]);
    int right = atoi(argv[3]);

    int data[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE; ++i)
        data[i] = atoi(argv[4 + i]);

    int signature = 0;

    /* Three feasible outcomes, including paths through a helper function. */
    int mode_class = classify(mode);
    if (mode_class < 0)
        signature += 1;
    else if (mode_class == 0)
        signature += 2;
    else
        signature += 3;

    /* Short-circuit && and || conditions. */
    if (left > right && right != 0)
        signature += 4;
    else if (left == right || right < 0)
        signature += 8;
    else
        signature += 16;

    /* Fixed-bound loop; int64_t avoids overflow when summing int32 inputs. */
    long long total = 0;
    for (int i = 0; i < ARRAY_SIZE; ++i)
        total += data[i];

    if (total < 0)
        signature += 32;
    else if (total == 0)
        signature += 64;
    else
        signature += 128;

    if ((data[0] % 2) == 0)
        signature += 256;
    else
        signature += 512;

    if (data[1] > data[2] && data[3] != 0)
        signature += 1024;
    else
        signature += 2048;

    /* Print the path signature so native replay can show the observed outcome. */
    printf("signature=%d\n", signature);
    return 0;
}
