/* implicit_runtime.c */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sanitizer/dfsan_interface.h>

#define NAME_BUF 4096

static __thread int active = 0;
static __thread uint8_t current_label = 0;
static __thread uint32_t current_line = 0;
static __thread uint32_t current_column = 0;

static __thread char variable_names[NAME_BUF];
static __thread size_t names_used = 0;

static void reset_state(void)
{
    active = 0;
    current_label = 0;
    current_line = 0;
    current_column = 0;
    names_used = 0;
    variable_names[0] = '\0';
}

static void flush_state(void)
{
    if (!active || names_used == 0)
        return;

    fprintf(
        stderr,
        "line=%u: col=%u, tainted variables - %s\n",
        current_line,
        current_column,
        variable_names
    );

    reset_state();
}

static int contains_variable(const char *name)
{
    if (!name || !*name)
        return 1;

    const char *p = variable_names;

    while (*p != '\0') {
        const char *start = p;

        while (*p != '\0' && *p != ',')
            ++p;

        size_t len = (size_t)(p - start);
        size_t wanted = strlen(name);

        if (len == wanted &&
            strncmp(start, name, wanted) == 0) {
            return 1;
        }

        if (*p == ',')
            ++p;
    }

    return 0;
}

static void add_variable(const char *name)
{
    if (!name || !*name)
        return;

    if (contains_variable(name))
        return;

    size_t len = strlen(name);

    size_t extra =
        len + (names_used ? 2 : 0);

    if (names_used + extra >= NAME_BUF)
        return;

    if (names_used) {
        variable_names[names_used++] = ',';
        variable_names[names_used++] = ' ';
    }

    memcpy(
        variable_names + names_used,
        name,
        len
    );

    names_used += len;
    variable_names[names_used] = '\0';
}

static void implicit_runtime_exit(void)
{
    flush_state();
}

__attribute__((constructor))
static void implicit_runtime_init(void)
{
    atexit(implicit_runtime_exit);
}

void __implicit_branch_callback(
    uint8_t condition_label,
    uint32_t line,
    uint32_t column,
    const char *variable,
    const void *address,
    size_t size)
{
    if (condition_label == 0)
        return;

    if (!address || size == 0)
        return;

    dfsan_label variable_label =
        dfsan_read_label(address, size);

    if (variable_label == 0)
        return;

    if (!dfsan_has_label(
            condition_label,
            variable_label)) {
        return;
    }

    if (!active ||
        current_label != condition_label ||
        current_line != line ||
        current_column != column) {

        flush_state();

        active = 1;
        current_label = condition_label;
        current_line = line;
        current_column = column;
        names_used = 0;
        variable_names[0] = '\0';
    }

    add_variable(variable);
}