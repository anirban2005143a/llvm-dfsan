#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#include <sanitizer/dfsan_interface.h>

void __implicit_branch_callback(
    dfsan_label condition_label,
    uint32_t line,
    uint32_t column,
    const char *variable,
    const void *address,
    uint64_t size)
{
    if (condition_label == 0)
        return;

    if (address == NULL || size == 0)
        return;

    dfsan_label variable_label =
        dfsan_read_label(
            address,
            (size_t)size);

    if (variable_label == 0)
        return;

    if (!dfsan_has_label(
            condition_label,
            variable_label))
        return;

    printf(
        "line=%u col=%u variable=%s\n",
        line,
        column,
        variable ? variable : "unknown");
}