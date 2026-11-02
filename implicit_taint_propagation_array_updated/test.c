#include <sanitizer/dfsan_interface.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    // Three public inputs
    int public1 = atoi(argv[1]);
    int public2 = atoi(argv[2]);
    int n = atoi(argv[3]);

    if (n < 1 || argc != 6 + n)
        return 1;

    // Two tainted scalar inputs
    int secret1 = atoi(argv[4]);
    int secret2 = atoi(argv[5]);

    // Dynamic array
    int arr[n];

    for (int i = 0; i < n; i++)
        arr[i] = atoi(argv[6 + i]);

    // Apply taint labels
    dfsan_set_label(4, &secret1, sizeof(secret1));
    dfsan_set_label(4, &secret2, sizeof(secret2));
    // dfsan_set_label(4, arr, sizeof(arr));

    // Conditional array updates using public and secret inputs
    for (int i = 0; i < n; i++) {
        if(i < n/2) {
            arr[i] = 20;
            continue;
        }
        if (public1 != 0 && secret1 > 0) {
            arr[i] = arr[i] + secret1;
        } 
        else if (public2 != 0 && secret2 < 0) {
            arr[i] = arr[i] + public1;
        } 
        else if (i % 2 == 0) {
            arr[i] = 20;
        } 
        else {
            arr[i] = 10;
        }
    }

    return 0;
}