// FILE: implicit_runtime.c
// REPLACE THE ENTIRE FILE WITH THIS

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sanitizer/dfsan_interface.h>

#define MAX_JOINS 4096

typedef struct {
    uint32_t join_id;
    dfsan_label pending_label;
    dfsan_label control_mask;
    uint64_t path_hash;
    int active;
} JoinState;

static JoinState States[MAX_JOINS];

static uint64_t initial_hash(void)
{
    return 1469598103934665603ULL;
}

static uint64_t hash_edge(
    uint64_t Hash,
    uint32_t BranchID,
    uint8_t Taken)
{
    Hash ^= BranchID;
    Hash *= 1099511628211ULL;

    Hash ^= Taken;
    Hash *= 1099511628211ULL;

    return Hash;
}

static JoinState *get_state(
    uint32_t JoinID)
{
    for (size_t I = 0;
         I < MAX_JOINS;
         ++I) {

        if (States[I].join_id == JoinID)
            return &States[I];
    }

    for (size_t I = 0;
         I < MAX_JOINS;
         ++I) {

        if (States[I].join_id == 0) {

            States[I].join_id = JoinID;
            States[I].path_hash =
                initial_hash();

            return &States[I];
        }
    }

    return NULL;
}

static int is_apply_mode(void)
{
    const char *Mode =
        getenv("IMPLICIT_MODE");

    return Mode &&
           strcmp(Mode, "apply") == 0;
}

static uint64_t read_u64(
    void *Address,
    uint32_t Size)
{
    uint64_t Value = 0;

    if (Size > sizeof(Value))
        Size = sizeof(Value);

    memcpy(
        &Value,
        Address,
        Size);

    return Value;
}

void __implicit_branch_begin(
    uint32_t JoinID,
    uint32_t BranchID,
    dfsan_label ConditionLabel)
{
    (void)BranchID;

    JoinState *State =
        get_state(JoinID);

    if (!State)
        return;

    State->pending_label =
        ConditionLabel;

    State->active = 1;
}

void __implicit_branch_edge(
    uint32_t JoinID,
    uint32_t BranchID,
    uint8_t Taken)
{
    JoinState *State =
        get_state(JoinID);

    if (!State)
        return;

    if (State->path_hash == 0)
        State->path_hash =
            initial_hash();

    State->control_mask |=
        State->pending_label;

    State->path_hash =
        hash_edge(
            State->path_hash,
            BranchID,
            Taken);

    State->pending_label = 0;
}

void __implicit_join_value(
    uint32_t JoinID,
    uint32_t VariableID,
    uint32_t Line,
    uint32_t Col,
    const char *Variable,
    uint64_t Value,
    uint32_t Size,
    void *Address)
{
    JoinState *State =
        get_state(JoinID);

    if (!State)
        return;

    const char *Log =
        getenv("IMPLICIT_LOG");

    if (!Log || Log[0] == '\0')
        Log = "observations.tsv";

    if (!is_apply_mode()) {

        const char *TestID =
            getenv("IMPLICIT_TEST_ID");

        if (!TestID)
            TestID = "0";

        FILE *File =
            fopen(Log, "a");

        if (File) {

            fprintf(
                File,
                "%s\t"
                "%u\t"
                "%u\t"
                "%s\t"
                "%u\t"
                "%u\t"
                "%llu\t"
                "%u\t"
                "%llu\n",

                TestID,
                JoinID,
                VariableID,
                Variable
                    ? Variable
                    : "unknown",
                Line,
                Col,
                (unsigned long long)
                    State->path_hash,
                (unsigned)
                    State->control_mask,
                (unsigned long long)
                    Value);

            fclose(File);
        }

        return;
    }

    if (!Address || Size == 0)
        return;

    const char *Map =
        getenv("IMPLICIT_TAINT_MAP");

    if (!Map || Map[0] == '\0')
        Map = "taint_map.txt";

    FILE *File =
        fopen(Map, "r");

    if (!File)
        return;

    char Buffer[256];

    while (fgets(
        Buffer,
        sizeof(Buffer),
        File)) {

        unsigned MapJoin = 0;
        unsigned MapVariable = 0;
        unsigned Label = 0;

        if (sscanf(
                Buffer,
                "%u%u%u",
                &MapJoin,
                &MapVariable,
                &Label) != 3) {
            continue;
        }

        if (MapJoin != JoinID ||
            MapVariable != VariableID) {
            continue;
        }

        dfsan_add_label(
            (dfsan_label)Label,
            Address,
            Size);

        printf(
            "line=%u col=%u variable=%s label=%u\n",
            Line,
            Col,
            Variable
                ? Variable
                : "unknown",
            Label);

        break;
    }

    fclose(File);

    (void)read_u64;
}

void __implicit_join_end(
    uint32_t JoinID)
{
    JoinState *State =
        get_state(JoinID);

    if (!State)
        return;

    State->pending_label = 0;
    State->control_mask = 0;
    State->path_hash =
        initial_hash();
    State->active = 0;
}