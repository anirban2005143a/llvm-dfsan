#include <sanitizer/dfsan_interface.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CURRENT_VARIABLES 4096
#define MAX_FRAMES 64
#define MAX_SCOPE_NAME 128
#define MAX_VARIABLE_NAME 128
#define MAX_BRANCH_STATES 4096
#define MAX_CONTEXT_DEPENDENCIES 512

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
};

struct BranchState {
    uint32_t id;
    dfsan_label label;
    uint64_t observed_outcomes;
    int valid;
};

static struct Frame Frames[MAX_FRAMES];
static size_t FrameCount;
static uint64_t NextFrameId = 1;

static struct CurrentVariable Current[MAX_CURRENT_VARIABLES];
static size_t CurrentCount;

static struct BranchState Branches[MAX_BRANCH_STATES];
static size_t BranchCount;

static dfsan_label PendingConditionLabel;
static FILE *TraceFP;
static int Initialized;

static void finalize_runtime(void);

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

static int same_text(const char *A, const char *B) {
    return A && B && strcmp(A, B) == 0;
}

static struct CurrentVariable *find_current_variable(uint64_t frame_id,
                                                       const char *scope,
                                                       const char *name) {
    for (size_t i = 0; i < CurrentCount; ++i) {
        struct CurrentVariable *V = &Current[i];
        if (V->frame_id != frame_id)
            continue;
        if (!same_text(V->scope, scope) || !same_text(V->name, name))
            continue;
        return V;
    }
    return NULL;
}

static struct BranchState *find_branch_state(uint32_t id) {
    for (size_t i = 0; i < BranchCount; ++i) {
        if (Branches[i].id == id)
            return &Branches[i];
    }
    return NULL;
}

static struct BranchState *get_or_create_branch_state(uint32_t id) {
    struct BranchState *B = find_branch_state(id);
    if (B)
        return B;

    if (BranchCount >= MAX_BRANCH_STATES)
        return NULL;

    B = &Branches[BranchCount++];
    memset(B, 0, sizeof(*B));
    B->id = id;
    B->valid = 1;
    return B;
}

static void write_hex(FILE *FP, const unsigned char *Data, size_t Size) {
    static const char Hex[] = "0123456789abcdef";

    if (!Data || Size == 0) {
        fputc('-', FP);
        return;
    }

    for (size_t i = 0; i < Size; ++i) {
        unsigned char C = Data[i];
        fputc(Hex[C >> 4], FP);
        fputc(Hex[C & 0x0f], FP);
    }
}

static dfsan_label context_label(const uint32_t *Dependencies, size_t Count) {
    dfsan_label Label = 0;

    if (!Dependencies)
        return 0;

    size_t Limit = Count;
    if (Limit > MAX_CONTEXT_DEPENDENCIES)
        Limit = MAX_CONTEXT_DEPENDENCIES;

    for (size_t i = 0; i < Limit; ++i) {
        struct BranchState *B = find_branch_state(Dependencies[i]);
        if (B && B->valid)
            Label = (dfsan_label)(Label | B->label);
    }

    return Label;
}

static void trace_observation(const struct CurrentVariable *V,
                              uint32_t BranchId, uint32_t Outcome,
                              dfsan_label ContextLabel) {
    if (!TraceFP || !V || !V->addr || V->size == 0)
        return;

    fprintf(TraceFP, "O|%u|%u|%u|%u|%zu|%s|%s|",
            BranchId,
            Outcome,
            (unsigned)ContextLabel,
            V->flags,
            V->size,
            V->scope,
            V->name);
    write_hex(TraceFP, (const unsigned char *)V->addr, V->size);
    fputc('\n', TraceFP);
}

static void trace_final(const struct CurrentVariable *V) {
    if (!TraceFP || !V || !V->addr || V->size == 0)
        return;

    dfsan_label ExplicitLabel = dfsan_read_label(V->addr, V->size);

    fprintf(TraceFP, "F|%u|%zu|%u|%s|%s|",
            V->flags,
            V->size,
            (unsigned)ExplicitLabel,
            V->scope,
            V->name);
    write_hex(TraceFP, (const unsigned char *)V->addr, V->size);
    fputc('\n', TraceFP);
}

static void conditional_callback(dfsan_label label, dfsan_origin origin) {
    (void)origin;
    PendingConditionLabel = label;
}

static void initialize_runtime(void) {
    if (Initialized)
        return;

    Initialized = 1;
    TraceFP = fopen(trace_path(), "ab");
    if (!TraceFP) {
        fprintf(stderr, "error: cannot open IMPLICIT_TRACE_FILE: %s\n",
                trace_path());
        exit(1);
    }

    dfsan_set_conditional_callback(conditional_callback);
}

__attribute__((constructor)) static void implicit_runtime_constructor(void) {
    initialize_runtime();
    atexit(finalize_runtime);
}

void __implicit_enter_function(const char *scope) {
    initialize_runtime();

    if (FrameCount >= MAX_FRAMES)
        return;

    struct Frame *Frame = &Frames[FrameCount++];
    memset(Frame, 0, sizeof(*Frame));
    Frame->id = NextFrameId++;
    copy_string(Frame->scope, sizeof(Frame->scope), scope);

    PendingConditionLabel = 0;
}

void __implicit_exit_function(const char *scope) {
    initialize_runtime();
    (void)scope;

    if (FrameCount == 0)
        return;

    uint64_t FrameId = Frames[FrameCount - 1].id;

    size_t i = 0;
    while (i < CurrentCount) {
        struct CurrentVariable *V = &Current[i];
        if (V->frame_id != FrameId) {
            ++i;
            continue;
        }

        trace_final(V);
        Current[i] = Current[CurrentCount - 1];
        --CurrentCount;
    }

    --FrameCount;
    PendingConditionLabel = 0;
    fflush(TraceFP);
}

void __implicit_register_variable(const char *scope, const char *name,
                                  void *addr, size_t size, uint32_t flags) {
    initialize_runtime();

    if (FrameCount == 0 || !scope || !name || !addr || size == 0)
        return;

    uint64_t FrameId = Frames[FrameCount - 1].id;
    struct CurrentVariable *V =
        find_current_variable(FrameId, scope, name);

    if (!V) {
        if (CurrentCount >= MAX_CURRENT_VARIABLES)
            return;

        V = &Current[CurrentCount++];
        memset(V, 0, sizeof(*V));
        V->frame_id = FrameId;
        copy_string(V->scope, sizeof(V->scope), scope);
        copy_string(V->name, sizeof(V->name), name);
    }

    V->addr = addr;
    V->size = size;
    V->flags = flags;
}

void __implicit_clear_condition(void) {
    initialize_runtime();
    PendingConditionLabel = 0;
}

void __implicit_record_condition(uint32_t branch_id) {
    initialize_runtime();

    struct BranchState *B = get_or_create_branch_state(branch_id);
    if (!B)
        return;

    B->label = PendingConditionLabel;
    B->valid = 1;
    PendingConditionLabel = 0;
}

void __implicit_record_branch_outcome(uint32_t branch_id,
                                      uint32_t outcome) {
    initialize_runtime();

    struct BranchState *B = get_or_create_branch_state(branch_id);
    if (!B)
        return;

    if (outcome < 64)
        B->observed_outcomes |= (UINT64_C(1) << outcome);
}

void __implicit_observe_branch(uint32_t branch_id,
                               const uint32_t *dependencies,
                               size_t dependency_count) {
    initialize_runtime();

    if (FrameCount == 0)
        return;

    struct BranchState *B = find_branch_state(branch_id);
    if (!B || !B->valid || B->observed_outcomes == 0)
        return;

    dfsan_label Label = context_label(dependencies, dependency_count);

    uint64_t Outcomes = B->observed_outcomes;
    for (uint32_t Outcome = 0; Outcome < 64; ++Outcome) {
        if (!(Outcomes & (UINT64_C(1) << Outcome)))
            continue;

        for (size_t i = 0; i < CurrentCount; ++i) {
            struct CurrentVariable *V = &Current[i];
            if (V->frame_id != Frames[FrameCount - 1].id)
                continue;

            if (V->flags & 1u)
                continue;

            trace_observation(V, branch_id, Outcome, Label);
        }
    }

    B->observed_outcomes = 0;
}

/* Required by -dfsan-event-callbacks. The analysis deliberately does not
 * modify DFSan shadow memory; concrete values are compared across paths. */
void __dfsan_load_callback(dfsan_label label, void *addr) {
    (void)label;
    (void)addr;
}

void __dfsan_store_callback(dfsan_label label, void *addr) {
    (void)label;
    (void)addr;
}

void __dfsan_mem_transfer_callback(dfsan_label *start, size_t len) {
    (void)start;
    (void)len;
}

void __dfsan_cmp_callback(dfsan_label combined_label) {
    (void)combined_label;
}

static void finalize_runtime(void) {
    if (!Initialized || !TraceFP)
        return;

    for (size_t i = 0; i < CurrentCount; ++i) {
        struct CurrentVariable *V = &Current[i];
        if (V->frame_id == Frames[FrameCount ? FrameCount - 1 : 0].id)
            trace_final(V);
    }

    fflush(TraceFP);
    fclose(TraceFP);
    TraceFP = NULL;
}
