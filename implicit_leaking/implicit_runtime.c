#include <stdint.h>
#include <stdio.h>

#include <sanitizer/dfsan_interface.h>

void __implicit_branch_callback(
    dfsan_label condition_label,
    dfsan_label variable_label,
    uint32_t line,
    uint32_t column,
    const char *variable)
{
    if (condition_label == 0)
        return;

    if (variable_label == 0)
        return;

    if (!dfsan_has_label(
            condition_label,
            variable_label))
        return;

    if (variable == NULL)
        return;

    fprintf(
        stderr,
        "line=%u col=%u variable=%s\n",
        line,
        column,
        variable);
}
