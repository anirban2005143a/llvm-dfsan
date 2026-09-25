#include <stdint.h>
#include <stdio.h>

void __implicit_branch_callback(
    uint8_t label,
    uint32_t line,
    uint32_t column,
    uint32_t loop_count,
    const char *loop_names,
    const int64_t *loop_values,
    const char *variable) {

    if (label == 0)
        return;

    fprintf(stderr, "line=%u:%u", line, column);

    if (loop_count > 0 &&
        loop_names != NULL &&
        loop_values != NULL) {

        const char *name = loop_names;

        for (uint32_t i = 0; i < loop_count; ++i) {

            const char *end = name;

            while (*end != '\0' && *end != ',')
                ++end;

            fprintf(stderr,
                    " %.*s=%lld",
                    (int)(end - name),
                    name,
                    (long long)loop_values[i]);

            if (*end == ',')
                name = end + 1;
            else
                name = end;
        }
    }

    fprintf(stderr,
            " variable=%s\n",
            variable ? variable : "unknown");
}