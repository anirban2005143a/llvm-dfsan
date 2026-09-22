#include <sanitizer/dfsan_interface.h>

#include <cstdint>
#include <cstdio>

// ------------------------------------------------------------
// Print the label and source contribution of every byte.
// ------------------------------------------------------------
void check_output(
    const char *name,
    const uint8_t *data,
    size_t size,
    const dfsan_label *source_labels,
    const char *const *source_names,
    size_t num_sources
) {
    printf("\n====================================================\n");
    printf("Checking output: %s\n", name);
    printf("Total size: %zu bytes\n", size);
    printf("====================================================\n");

    size_t total_tainted = 0;

    for (size_t i = 0; i < size; i++) {

        // Read DFSan label of exactly ONE byte.
        dfsan_label label =
            dfsan_read_label(&data[i], 1);

        printf("\nByte %zu\n", i);
        printf("  Value : 0x%02X\n", data[i]);
        printf("  Label : %u (0x%02X)\n",
               label, label);

        if (label == 0) {
            printf("  Origin: CLEAN\n");
            continue;
        }

        total_tainted++;

        bool has_source = false;

        // Check every possible source.
        for (size_t j = 0; j < num_sources; j++) {

            if (dfsan_has_label(label, source_labels[j])) {

                printf("  Source: %s\n",
                       source_names[j]);

                has_source = true;
            }
        }

        if (!has_source) {
            printf("  Source: UNKNOWN\n");
        }
    }

    printf("\n----------------------------------------------------\n");
    printf("Summary\n");
    printf("----------------------------------------------------\n");
    printf("Total bytes     : %zu\n", size);
    printf("Tainted bytes   : %zu\n", total_tainted);
    printf("Clean bytes     : %zu\n", size - total_tainted);
    printf("----------------------------------------------------\n");
}


int main() {

    // ========================================================
    // 1. Create two completely tainted input variables
    // ========================================================

    uint32_t x = 0x11223344;
    uint32_t y = 0x55667788;

    // Different labels = different taint sources.
    dfsan_label x_label = 1;
    dfsan_label y_label = 2;

    dfsan_set_label(
        x_label,
        &x,
        sizeof(x)
    );

    dfsan_set_label(
        y_label,
        &y,
        sizeof(y)
    );

    printf("====================================================\n");
    printf("TAINT SOURCES\n");
    printf("====================================================\n");

    printf("X = 0x%08X\n", x);
    printf("X label = %u\n", x_label);

    printf("\nY = 0x%08X\n", y);
    printf("Y label = %u\n", y_label);


    // ========================================================
    // 2. Create Z
    //
    // z[0] = one byte from X
    // z[1] = one byte from Y
    // z[2] = one byte from X + one byte from Y
    // z[3] = constant
    // ========================================================

    uint8_t z[4];

    // --------------------------------------------------------
    // Byte 0:
    // Take one byte from X.
    //
    // X = 0x11 22 33 44
    // On little-endian:
    // X byte 0 = 0x44
    // --------------------------------------------------------

    z[0] = ((uint8_t *)&x)[0];


    // --------------------------------------------------------
    // Byte 1:
    // Take one byte from Y.
    //
    // Y = 0x55 66 77 88
    // Y byte 0 = 0x88
    // --------------------------------------------------------

    z[1] = ((uint8_t *)&y)[0];


    // --------------------------------------------------------
    // Byte 2:
    // One byte from X + one byte from Y.
    //
    // X byte 1 = 0x33
    // Y byte 1 = 0x77
    //
    // 0x33 + 0x77 = 0xAA
    //
    // Both X and Y should contribute their labels.
    // --------------------------------------------------------

    z[2] =
        ((uint8_t *)&x)[1] +
        ((uint8_t *)&y)[1];


    // --------------------------------------------------------
    // Byte 3:
    // Completely independent constant.
    // --------------------------------------------------------

    z[3] = 0xCC;


    // ========================================================
    // 3. Print the expected construction
    // ========================================================

    printf("\n====================================================\n");
    printf("EXPECTED DATA FLOW\n");
    printf("====================================================\n");

    printf("z[0] = X byte 0\n");
    printf("z[1] = Y byte 0\n");
    printf("z[2] = X byte 1 + Y byte 1\n");
    printf("z[3] = CONSTANT 0xCC\n");


    // ========================================================
    // 4. Check Z
    // ========================================================

    dfsan_label source_labels[] = {
        x_label,
        y_label
    };

    const char *source_names[] = {
        "X",
        "Y"
    };

    check_output(
        "Z",
        z,
        sizeof(z),
        source_labels,
        source_names,
        2
    );


    return 0;
}
