#include <stdio.h>
#include <sanitizer/dfsan_interface.h>

int main() {
    int x;

    printf("Enter a number: ");
    scanf("%d", &x);

    // Assign label 42 to x
    dfsan_set_label(42, &x, sizeof(x));

    // Check x
    dfsan_label label_x = dfsan_get_label(x);

    printf("\n--- x information ---\n");
    printf("Value  : %d\n", x);
    printf("Label  : %u\n", label_x);
    printf("Tainted: %s\n", label_x ? "YES" : "NO");

    // Create another variable from x
    int y = x + 10;

    dfsan_label label_y = dfsan_get_label(y);

    printf("\n--- y information ---\n");
    printf("Value  : %d\n", y);
    printf("Label  : %u\n", label_y);
    printf("Tainted: %s\n", label_y ? "YES" : "NO");

    return 0;
}
