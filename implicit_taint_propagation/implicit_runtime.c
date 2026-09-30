/* implicit_runtime.c */

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
            variable_label,
            condition_label))
        return;

    fprintf(
        stderr,
        "[IMPLICIT TAINT] "
        "line=%u col=%u "
        "variable=%s "
        "condition_label=%u "
        "variable_label=%u\n",
        line,
        column,
        variable ? variable : "unknown",
        (unsigned)condition_label,
        (unsigned)variable_label);
}