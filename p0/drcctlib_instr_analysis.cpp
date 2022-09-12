/*
 *  Copyright (c) 2022 Xuhpclab. All rights reserved.
 *  Licensed under the MIT License.
 *  See LICENSE file for more information.
 */

#include "dr_api.h"
#include "drcctlib.h"
#include <map>
#include <list>
#include <set>
#include <iostream>
#include <bits/stdc++.h>
static file_t gTraceFile;

using namespace std;

#define DRCCTLIB_PRINTF(_FORMAT, _ARGS...) \
    DRCCTLIB_PRINTF_TEMPLATE("instr_statistics", _FORMAT, ##_ARGS)

static map<context_handle_t, int> memory_loads;
static map<context_handle_t, int> memory_stores;
static map<context_handle_t, int> conditional_branches;
static map<context_handle_t, int> unconditional_branches;

void
InsertCleancall(int32_t slot, int32_t cat)
{
    void *drcontext = dr_get_current_drcontext();
    context_handle_t handle = drcctlib_get_context_handle(drcontext, slot);
    if (cat & (1 << 0))
        unconditional_branches[handle] += 1;
    if (cat & (1 << 1))
        conditional_branches[handle] += 1;
    if (cat & (1 << 2))
        memory_stores[handle] += 1;
    if (cat & (1 << 3))
        memory_loads[handle] += 1;
}

// analysis
void
InstrumentInsCallback(void *drcontext, instr_instrument_msg_t *instrument_msg)
{
    int cat = 0;
    if (instr_is_ubr(instrument_msg->instr))
        cat |= (1 << 0);
    else if (instr_is_cbr(instrument_msg->instr))
        cat |= (1 << 1);
    else if (instr_writes_memory(instrument_msg->instr))
        cat |= (1 << 2);
    else if (instr_reads_memory(instrument_msg->instr))
        cat |= (1 << 3);

    if (cat != 0) {
        dr_insert_clean_call(
            drcontext, instrument_msg->bb, instrument_msg->instr, (void *)InsertCleancall,
            false, 2, OPND_CREATE_INT32(instrument_msg->slot), OPND_CREATE_INT32(cat));
    }
}

struct comp {
    template <typename T>
    bool
    operator()(const T &l, const T &r) const
    {
        return l.second > r.second;
    }
};

static void
ClientInit(int argc, const char *argv[])
{
    char name[MAXIMUM_FILEPATH] = "";
    DRCCTLIB_INIT_LOG_FILE_NAME(name, "instr_statistics_clean_call", "out");
    DRCCTLIB_PRINTF("Creating log file at:%s", name);

    drcctlib_init(DRCCTLIB_FILTER_ALL_INSTR, INVALID_FILE, InstrumentInsCallback, false);
    gTraceFile = dr_open_file(name, DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
}

static void
PrintInfo(map<context_handle_t, int> *m, const char *title)
{

    list<int> values;
    int totalCount = 0;
    for (auto &t : *m) {
        values.push_back(t.second);
        totalCount += t.second;
    }

    dr_fprintf(gTraceFile, title);
    dr_fprintf(gTraceFile, " : %d\n", totalCount);
    set<int> uniqueValues(values.begin(), values.end());
    list<int> uniqueValuesList(uniqueValues.begin(), uniqueValues.end());
    uniqueValuesList.sort();
    uniqueValuesList.reverse();

    int printCount = 0;
    for (auto &t : uniqueValuesList) {
        if (printCount > 10)
            break;
        for (auto &t2 : *m) {
            if (printCount > 10)
                break;
            if (t2.second == t) {
                dr_fprintf(gTraceFile, "NO. %d PC ", printCount + 1);
                drcctlib_print_backtrace_first_item(gTraceFile, t2.first, true, false);
                dr_fprintf(gTraceFile, "=>EXECUTION TIMES\n%lld\n=>BACKTRACE\n",
                           t2.second);
                drcctlib_print_backtrace(gTraceFile, t2.first, false, true, -1);
                dr_fprintf(gTraceFile, "\n\n\n");
                printCount++;
            }
        }
    }
}

static void
ClientExit(void)
{
    string a = "MEMORY LOAD";
    PrintInfo(&memory_loads, a.c_str());
    a = "MEMORY STORE";
    PrintInfo(&memory_stores, a.c_str());
    a = "CONDITIONAL BRANCHES";
    PrintInfo(&conditional_branches, a.c_str());
    a = "UNCONDITIONAL BRANCHES";
    PrintInfo(&unconditional_branches, a.c_str());

    drcctlib_exit();
}

#ifdef __cplusplus
extern "C" {
#endif

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
    ClientInit(argc, argv);

    dr_register_exit_event(ClientExit);
}

#ifdef __cplusplus
}
#endif