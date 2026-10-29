#include <sanitizer/dfsan_interface.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CURRENT_VARIABLES 4096
#define MAX_FRAMES 64
#define MAX_SCOPE_NAME 128
#define MAX_VARIABLE_NAME 128
#define MAX_IMPLICIT_MARKS 16384

#define VARIABLE_FLAG_FLOATING 2u
#define VARIABLE_FLAG_GLOBAL 8u

struct Frame {
    uint64_t id;
    char scope[MAX_SCOPE_NAME];
};

struct CurrentVariable {
    uint64_t frame_id;
    char scope[MAX_SCOPE_NAME];
    char name[MAX_VARIABLE_NAME];
    void *addr;
    size_t size;
    uint32_t flags;
    int initialized;
};

struct PlannedImplicitLabel {
    dfsan_label label;
    char scope[MAX_SCOPE_NAME];
    char name[MAX_VARIABLE_NAME];
};

static struct Frame Frames[MAX_FRAMES];
static size_t FrameCount;
static uint64_t NextFrameId = 1;

static struct CurrentVariable Current[MAX_CURRENT_VARIABLES];
static size_t CurrentCount;

static struct PlannedImplicitLabel ImplicitMarks[MAX_IMPLICIT_MARKS];
static size_t ImplicitMarkCount;
static int ApplyImplicitPlan;

static FILE *TraceFP;
static int Initialized;

static void finalize_runtime(void);
static void apply_final_implicit_labels(void);
static void copy_string(char *dst, size_t dst_size, const char *src);

static void load_implicit_plan(void) {
    const char *path = getenv("IMPLICIT_LABELS_FILE");
    if (!path || !*path)
        return;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "error: cannot open IMPLICIT_LABELS_FILE: %s\n", path);
        exit(1);
    }

    ApplyImplicitPlan = 1;
    char line[2 * MAX_VARIABLE_NAME + MAX_SCOPE_NAME + 64];
    while (fgets(line, sizeof(line), fp)) {
        char stage[32] = {0};
        unsigned label = 0;
        char scope[MAX_SCOPE_NAME] = {0};
        char name[MAX_VARIABLE_NAME] = {0};

        if (sscanf(line, "%31[^|]|%u|%127[^|]|%127[^\r\n]",
                   stage, &label, scope, name) != 4)
            continue;

        // Value-comparison inference only creates EXIT entries. Branch-specific
        // plan records belonged to the previous control-dependence design.
        if (strcmp(stage, "EXIT") != 0 || label == 0 || label > UINT8_MAX)
            continue;
        if (ImplicitMarkCount >= MAX_IMPLICIT_MARKS) {
            fprintf(stderr, "error: too many implicit-label plan entries\n");
            fclose(fp);
            exit(1);
        }

        struct PlannedImplicitLabel *mark = &ImplicitMarks[ImplicitMarkCount++];
        memset(mark, 0, sizeof(*mark));
        mark->label = (dfsan_label)label;
        copy_string(mark->scope, sizeof(mark->scope), scope);
        copy_string(mark->name, sizeof(mark->name), name);
    }

    fclose(fp);
}

static const char *trace_path(void) {
    const char *p = getenv("IMPLICIT_TRACE_FILE");
    return p && *p ? p : "/tmp/dfsan_implicit_trace.txt";
}

static void copy_string(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0)
        return;
    if (!src)
        src = "";
    snprintf(dst, dst_size, "%s", src);
}

static int same_text(const char *a, const char *b) {
    return a && b && strcmp(a, b) == 0;
}

static struct CurrentVariable *find_current_variable(uint64_t frame_id,
                                                       const char *scope,
                                                       const char *name) {
    for (size_t i = 0; i < CurrentCount; ++i) {
        struct CurrentVariable *v = &Current[i];
        if (v->frame_id != frame_id)
            continue;
        if (!same_text(v->scope, scope) || !same_text(v->name, name))
            continue;
        return v;
    }
    return NULL;
}

static void write_hex(FILE *fp, const unsigned char *data, size_t size) {
    static const char hex[] = "0123456789abcdef";
    if (!data || size == 0) {
        fputc('-', fp);
        return;
    }

    for (size_t i = 0; i < size; ++i) {
        unsigned char c = data[i];
        fputc(hex[c >> 4], fp);
        fputc(hex[c & 0x0f], fp);
    }
}

static void trace_final(const struct CurrentVariable *v) {
    if (!TraceFP || !v || !v->addr || v->size == 0 || !v->initialized)
        return;

    dfsan_label label = dfsan_read_label(v->addr, v->size);
    fprintf(TraceFP, "F|%u|%zu|%u|%s|%s|",
            v->flags, v->size, (unsigned)label, v->scope, v->name);
    write_hex(TraceFP, (const unsigned char *)v->addr, v->size);
    fputc('\n', TraceFP);
}

static void initialize_runtime(void) {
    if (Initialized)
        return;

    Initialized = 1;
    load_implicit_plan();
    TraceFP = fopen(trace_path(), "ab");
    if (!TraceFP) {
        fprintf(stderr, "error: cannot open IMPLICIT_TRACE_FILE: %s\n",
                trace_path());
        exit(1);
    }
}

__attribute__((constructor)) static void implicit_runtime_constructor(void) {
    initialize_runtime();
    atexit(finalize_runtime);
}

void __implicit_enter_function(const char *scope) {
    initialize_runtime();
    if (FrameCount >= MAX_FRAMES)
        return;

    struct Frame *frame = &Frames[FrameCount++];
    memset(frame, 0, sizeof(*frame));
    frame->id = NextFrameId++;
    copy_string(frame->scope, sizeof(frame->scope), scope);
}

void __implicit_register_variable(const char *scope, const char *name,
                                  void *addr, size_t size, uint32_t flags) {
    initialize_runtime();
    if (FrameCount == 0 || !scope || !name || !addr || size == 0)
        return;

    uint64_t frame_id = Frames[FrameCount - 1].id;
    struct CurrentVariable *v = find_current_variable(frame_id, scope, name);
    if (!v) {
        if (CurrentCount >= MAX_CURRENT_VARIABLES)
            return;
        v = &Current[CurrentCount++];
        memset(v, 0, sizeof(*v));
        v->frame_id = frame_id;
        copy_string(v->scope, sizeof(v->scope), scope);
        copy_string(v->name, sizeof(v->name), name);
    }

    v->addr = addr;
    v->size = size;
    v->flags = flags;
    // Globals have static storage and are initialized before main begins.
    if (flags & VARIABLE_FLAG_GLOBAL)
        v->initialized = 1;
}

void __implicit_mark_initialized(void *addr, size_t size) {
    initialize_runtime();
    if (!addr || size == 0 || FrameCount == 0)
        return;

    uintptr_t begin = (uintptr_t)addr;
    if (size > UINTPTR_MAX - begin)
        return;
    uintptr_t end = begin + size;
    uint64_t frame_id = Frames[FrameCount - 1].id;

    // A scalar/arr[i] write marks only entries fully covered by that write.
    // Whole-array initialization marks each eligible element independently.
    for (size_t i = 0; i < CurrentCount; ++i) {
        struct CurrentVariable *v = &Current[i];
        if (v->frame_id != frame_id || !v->addr || v->size == 0)
            continue;
        uintptr_t value_begin = (uintptr_t)v->addr;
        if (v->size > UINTPTR_MAX - value_begin)
            continue;
        uintptr_t value_end = value_begin + v->size;
        if (value_begin >= begin && value_end <= end)
            v->initialized = 1;
    }
}

static void apply_final_implicit_labels(void) {
    if (!ApplyImplicitPlan || FrameCount == 0)
        return;

    uint64_t frame_id = Frames[FrameCount - 1].id;
    for (size_t i = 0; i < ImplicitMarkCount; ++i) {
        const struct PlannedImplicitLabel *mark = &ImplicitMarks[i];
        struct CurrentVariable *v =
            find_current_variable(frame_id, mark->scope, mark->name);
        if (!v || !v->addr || v->size == 0 || !v->initialized)
            continue;

        // Add the inferred label to this one scalar or array element. Existing
        // direct labels remain present because dfsan_add_label unions labels.
        dfsan_add_label(mark->label, v->addr, v->size);
    }
}

void __implicit_exit_function(const char *scope) {
    initialize_runtime();
    if (FrameCount == 0)
        return;
    if (scope && !same_text(Frames[FrameCount - 1].scope, scope))
        return;

    uint64_t frame_id = Frames[FrameCount - 1].id;

    // Implicit taint is inferred offline from discovery snapshots. In the
    // second run, apply the plan just before collecting the final snapshots.
    apply_final_implicit_labels();

    size_t i = 0;
    while (i < CurrentCount) {
        struct CurrentVariable *v = &Current[i];
        if (v->frame_id != frame_id) {
            ++i;
            continue;
        }

        trace_final(v);
        Current[i] = Current[CurrentCount - 1];
        --CurrentCount;
    }

    --FrameCount;
    fflush(TraceFP);
}

static void finalize_runtime(void) {
    if (!Initialized || !TraceFP)
        return;

    // Fallback for paths that terminate without reaching main's return hook.
    if (FrameCount > 0)
        apply_final_implicit_labels();
    for (size_t i = 0; i < CurrentCount; ++i)
        trace_final(&Current[i]);

    fflush(TraceFP);
    fclose(TraceFP);
    TraceFP = NULL;
}
