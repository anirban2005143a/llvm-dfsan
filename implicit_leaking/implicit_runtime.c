#include <stdio.h>
#include <stdint.h>
#include <sanitizer/dfsan_interface.h>

typedef struct {
    dfsan_label label;
    const char *name;
} source_label_t;

static const source_label_t source_labels[] = {
    {1, "secret1"},
    {2, "secret2"},
    {4, "secret3"},
};

static void print_contributors(dfsan_label label)
{
    int printed = 0;

    for (unsigned i = 0;
         i < sizeof(source_labels) / sizeof(source_labels[0]);
         ++i) {

        if (dfsan_has_label(label, source_labels[i].label)) {

            if (!printed) {
                printf("\nTainted contributors:\n");
                printed = 1;
            }

            printf("    %s -> label %u\n",
                   source_labels[i].name,
                   (unsigned)source_labels[i].label);
        }
    }

    if (!printed) {
        printf("\nTainted contributors:\n");
        printf("    unknown label mapping\n");
    }
}

void __implicit_conditional_callback(
    dfsan_label label,
    uint32_t condition_id,
    uint8_t taken,
    const char *file,
    uint32_t line,
    uint32_t column,
    const char *condition,
    const char *llvm_condition,
    const char *variables,
    const char *true_block,
    const char *false_block)
{
    if (label == 0)
        return;

    printf("\n");
    printf("================================================\n");
    printf("IMPLICIT TAINT FLOW\n");
    printf("================================================\n");

    printf("Condition ID : %u\n", condition_id);

    printf("\nSource position:\n");
    printf("    File   : %s\n", file ? file : "");
    printf("    Line   : %u\n", line);
    printf("    Column : %u\n", column);

    printf("\nCondition:\n");
    printf("    %s\n", condition ? condition : "");

    printf("\nLLVM condition:\n");
    printf("    %s\n", llvm_condition ? llvm_condition : "");

    printf("\nTainted variable candidate(s):\n");
    if (variables && variables[0])
        printf("    %s\n", variables);
    else
        printf("    <not resolved>\n");

    printf("\nDFSan label : %u\n", (unsigned)label);

    print_contributors(label);

    printf("\nContaminated branch:\n");

    if (taken) {
        printf("    TAKEN   -> %s\n", true_block ? true_block : "");
        printf("    other   -> %s\n", false_block ? false_block : "");
    } else {
        printf("    TAKEN   -> %s\n", false_block ? false_block : "");
        printf("    other   -> %s\n", true_block ? true_block : "");
    }

    printf("================================================\n");
}