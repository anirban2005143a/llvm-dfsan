#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sanitizer/dfsan_interface.h>

#define IMPLICIT_MAGIC 0x49544e54u
#define IMPLICIT_VERSION 1u
#define MAX_BRANCHES 64
#define MAX_HISTORY 128
#define MAX_VARS 32
#define NAME_SIZE 64

typedef struct {
    uint32_t id;
    uint8_t taken;
    dfsan_label label;
} BranchEvent;

typedef struct {
    int32_t value;
    dfsan_label explicit_label;
    size_t branch_count;
    BranchEvent branches[MAX_BRANCHES];
} CaseObservation;

typedef struct {
    char name[NAME_SIZE];
    dfsan_label inferred_label;
    size_t history_count;
    CaseObservation history[MAX_HISTORY];
} VariableState;

typedef struct {
    uint32_t magic;
    uint32_t version;
    size_t variable_count;
    VariableState variables[MAX_VARS];
} PersistentState;

typedef struct {
    char name[NAME_SIZE];
    int32_t *addr;
    int32_t value;
    dfsan_label explicit_label;
    size_t branch_count;
    BranchEvent branches[MAX_BRANCHES];
} CurrentObservation;

static PersistentState State;
static CurrentObservation Current[MAX_VARS];
static size_t CurrentCount;
static BranchEvent Path[MAX_BRANCHES];
static size_t PathCount;
static dfsan_label PendingConditionLabel;
static int Initialized;

static void finalize_runtime(void);

static const char *state_path(void) {
    const char *p = getenv("IMPLICIT_STATE_FILE");
    return p && *p ? p : "/tmp/dfsan_implicit_state.bin";
}

static void load_state(void) {
    memset(&State, 0, sizeof(State));

    FILE *fp = fopen(state_path(), "rb");
    if (!fp)
        return;

    PersistentState tmp;
    size_t n = fread(&tmp, sizeof(tmp), 1, fp);
    fclose(fp);

    if (n != 1)
        return;

    if (tmp.magic != IMPLICIT_MAGIC || tmp.version != IMPLICIT_VERSION)
        return;

    State = tmp;
}

static void save_state(void) {
    FILE *fp = fopen(state_path(), "wb");
    if (!fp)
        return;

    fwrite(&State, sizeof(State), 1, fp);
    fclose(fp);
}

static VariableState *find_state_variable(const char *name) {
    for (size_t i = 0; i < State.variable_count; ++i) {
        if (strcmp(State.variables[i].name, name) == 0)
            return &State.variables[i];
    }

    if (State.variable_count >= MAX_VARS)
        return NULL;

    VariableState *V = &State.variables[State.variable_count++];
    memset(V, 0, sizeof(*V));
    snprintf(V->name, sizeof(V->name), "%s", name);
    return V;
}

static CurrentObservation *find_current_observation(const char *name) {
    for (size_t i = 0; i < CurrentCount; ++i) {
        if (strcmp(Current[i].name, name) == 0)
            return &Current[i];
    }

    if (CurrentCount >= MAX_VARS)
        return NULL;

    CurrentObservation *C = &Current[CurrentCount++];
    memset(C, 0, sizeof(*C));
    snprintf(C->name, sizeof(C->name), "%s", name);
    return C;
}

static const BranchEvent *find_event(const BranchEvent *events, size_t count,
                                     uint32_t id) {
    for (size_t i = 0; i < count; ++i) {
        if (events[i].id == id)
            return &events[i];
    }
    return NULL;
}

static int has_event(const BranchEvent *events, size_t count, uint32_t id) {
    return find_event(events, count, id) != NULL;
}

static dfsan_label changed_control_labels(const CaseObservation *A,
                                          const CaseObservation *B) {
    dfsan_label result = 0;

    for (size_t i = 0; i < A->branch_count; ++i) {
        const BranchEvent *EA = &A->branches[i];
        const BranchEvent *EB = find_event(B->branches, B->branch_count, EA->id);

        if (!EB) {
            result |= EA->label;
            continue;
        }

        if (EA->taken != EB->taken)
            result |= (dfsan_label)(EA->label | EB->label);
    }

    for (size_t i = 0; i < B->branch_count; ++i) {
        const BranchEvent *EB = &B->branches[i];
        if (!has_event(A->branches, A->branch_count, EB->id))
            result |= EB->label;
    }

    return result;
}

static void conditional_callback(dfsan_label label, dfsan_origin origin) {
    (void)origin;
    PendingConditionLabel = label;
}

static void initialize_runtime(void) {
    if (Initialized)
        return;

    Initialized = 1;
    load_state();
    dfsan_set_conditional_callback(conditional_callback);
}

__attribute__((constructor)) static void implicit_runtime_constructor(void) {
    initialize_runtime();
    atexit(finalize_runtime);
}

void __implicit_clear_condition(void) {
    initialize_runtime();
    PendingConditionLabel = 0;
}

void __implicit_record_branch(uint32_t branch_id, uint8_t taken) {
    initialize_runtime();

    if (PathCount >= MAX_BRANCHES)
        return;

    Path[PathCount].id = branch_id;
    Path[PathCount].taken = taken ? 1 : 0;
    Path[PathCount].label = PendingConditionLabel;
    ++PathCount;

    PendingConditionLabel = 0;
}

void __implicit_observe_i32(const char *name, int32_t *addr) {
    initialize_runtime();

    if (!name || !addr)
        return;

    CurrentObservation *C = find_current_observation(name);
    if (!C)
        return;

    snprintf(C->name, sizeof(C->name), "%s", name);
    C->addr = addr;
    C->value = *addr;
    C->explicit_label = dfsan_read_label(addr, sizeof(*addr));
    C->branch_count = PathCount;

    if (PathCount > 0) {
        memcpy(C->branches, Path, PathCount * sizeof(Path[0]));
    }
}

static void finalize_runtime(void) {
    initialize_runtime();

    for (size_t i = 0; i < CurrentCount; ++i) {
        CurrentObservation *C = &Current[i];
        VariableState *V = find_state_variable(C->name);
        if (!V)
            continue;

        CaseObservation CurrentCase;
        memset(&CurrentCase, 0, sizeof(CurrentCase));
        CurrentCase.value = C->value;
        CurrentCase.explicit_label = C->explicit_label;
        CurrentCase.branch_count = C->branch_count;
        if (C->branch_count > 0) {
            memcpy(CurrentCase.branches, C->branches,
                   C->branch_count * sizeof(C->branches[0]));
        }

        for (size_t h = 0; h < V->history_count; ++h) {
            const CaseObservation *Previous = &V->history[h];
            if (Previous->value == CurrentCase.value)
                continue;

            V->inferred_label |=
                changed_control_labels(&CurrentCase, Previous);
        }

        dfsan_label final_label =
            (dfsan_label)(C->explicit_label | V->inferred_label);

        if (final_label != 0)
            dfsan_add_label(final_label, C->addr, sizeof(*C->addr));

        if (V->history_count < MAX_HISTORY)
            V->history[V->history_count++] = CurrentCase;
    }

    State.magic = IMPLICIT_MAGIC;
    State.version = IMPLICIT_VERSION;
    save_state();

    if (getenv("IMPLICIT_FINAL")) {
        const char *names[] = {"x", "y", "z", "a", "b", "c"};

        for (size_t n = 0; n < 3; ++n) {
            for (size_t i = 0; i < CurrentCount; ++i) {
                CurrentObservation *C = &Current[i];
                if (strcmp(C->name, names[n]) != 0)
                    continue;

                dfsan_label label = dfsan_read_label(C->addr, sizeof(*C->addr));
                printf("%s = %d, label = %u\n",
                       C->name, *C->addr, (unsigned)label);
                break;
            }
        }
    }
}

