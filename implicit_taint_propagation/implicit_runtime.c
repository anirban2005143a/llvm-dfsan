#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sanitizer/dfsan_interface.h>

typedef struct {
    dfsan_label *labels;
    size_t capacity;
} ImplicitState;

static _Thread_local ImplicitState State = {
    NULL,
    0
};

static void ensure_capacity(uint32_t ID)
{
    if ((size_t)ID < State.capacity)
        return;

    size_t NewCapacity =
        State.capacity ? State.capacity : 64;

    while (NewCapacity <= (size_t)ID)
        NewCapacity *= 2;

    dfsan_label *NewLabels =
        (dfsan_label *)realloc(
            State.labels,
            NewCapacity * sizeof(dfsan_label));

    if (!NewLabels)
        abort();

    memset(
        NewLabels + State.capacity,
        0,
        (NewCapacity - State.capacity) *
            sizeof(dfsan_label));

    State.labels = NewLabels;
    State.capacity = NewCapacity;
}

void __implicit_enter(
    uint32_t branch_id,
    dfsan_label condition_label)
{
    if (condition_label == 0)
        return;

    ensure_capacity(branch_id);

    /*
     * Keyed by branch ID instead of using a push/pop stack.
     *
     * This is important for loops:
     *
     *     while (secret) {
     *         ...
     *     }
     *
     * The same branch may be entered many times.
     * Re-entering overwrites the same slot instead of
     * endlessly growing a stack.
     */
    State.labels[branch_id] =
        condition_label;
}

void __implicit_leave(
    uint32_t branch_id)
{
    if ((size_t)branch_id >= State.capacity)
        return;

    State.labels[branch_id] = 0;
}

static int has_active_implicit_label(void)
{
    if (!State.labels)
        return 0;

    for (size_t I = 0;
         I < State.capacity;
         ++I) {

        if (State.labels[I] != 0)
            return 1;
    }

    return 0;
}

static int byte_has_active_label(
    dfsan_label byte_label)
{
    if (byte_label == 0)
        return 0;

    for (size_t I = 0;
         I < State.capacity;
         ++I) {

        dfsan_label Active =
            State.labels[I];

        if (Active == 0)
            continue;

        if (dfsan_has_label(
                byte_label,
                Active)) {

            return 1;
        }
    }

    return 0;
}

void __implicit_store_callback(
    void *address,
    size_t size,
    uint32_t line,
    uint32_t column,
    const char *variable)
{
    if (!address || size == 0)
        return;

    if (!has_active_implicit_label())
        return;

    /*
     * Apply every currently active control-flow label
     * to the bytes written by the executed store.
     */
    for (size_t I = 0;
         I < State.capacity;
         ++I) {

        dfsan_label Active =
            State.labels[I];

        if (Active == 0)
            continue;

        dfsan_add_label(
            Active,
            address,
            size);
    }

    /*
     * Count the bytes which now carry at least one
     * active implicit label.
     */
    size_t ImplicitBytes = 0;

    unsigned char *Bytes =
        (unsigned char *)address;

    for (size_t I = 0;
         I < size;
         ++I) {

        dfsan_label ByteLabel =
            dfsan_read_label(
                Bytes + I,
                1);

        if (byte_has_active_label(ByteLabel))
            ++ImplicitBytes;
    }

    if (ImplicitBytes == 0)
        return;

    fprintf(
        stderr,
        "[IMPLICIT TAINT] "
        "line=%u col=%u "
        "variable=%s "
        "bytes=%zu\n",
        line,
        column,
        variable ? variable : "unknown",
        ImplicitBytes);
}