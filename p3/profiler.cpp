
#include <iostream>
#include <bitset>
#include <cstdint>
#include <cmath>
#include "profiler.h"
#include "drcctlib_vscodeex_format.h"

using namespace std;
using namespace DrCCTProf;

/*
    Tips: different integer types have distinct boundaries
    INT64_MAX, INT32_MAX, INT16_MAX, INT8_MAX
    INT64_MIN, INT32_MIN, INT16_MIN, INT8_MIN
*/

namespace runtime_profiling {

static inline int64_t
GetOpndIntValue(Operand opnd)
{
    if (opnd.type == OperandType::kImmediateFloat ||
        opnd.type == OperandType::kImmediateDouble ||
        opnd.type == OperandType::kUnsupportOpnd || opnd.type == OperandType::kNullOpnd) {
        return 0;
    }
    int64_t value = 0;
    switch (opnd.size) {
    case 1: value = static_cast<int64_t>(opnd.value.valueInt8); break;
    case 2: value = static_cast<int64_t>(opnd.value.valueInt16); break;
    case 4: value = static_cast<int64_t>(opnd.value.valueInt32); break;
    case 8: value = static_cast<int64_t>(opnd.value.valueInt64); break;
    default: break;
    }
    return value;
}

static inline int64_t
GetMaxValue(Operand e)
{
    switch (e.size) {
    case 1: return INT8_MAX;
    case 2: return INT16_MAX;
    case 4: return INT32_MAX;
    default: return INT64_MAX;
    }
}
static inline int64_t
GetMinValue(Operand e)
{
    switch (e.size) {
    case 1: return INT8_MIN;
    case 2: return INT16_MIN;
    case 4: return INT32_MIN;
    default: return INT64_MIN;
    }
}

static inline int64_t
GetIntValue(Operand e)
{
    switch (e.size) {
    case 1: return e.value.valueInt8;
    case 2: return e.value.valueInt16;
    case 4: return e.value.valueInt32;
    default: return e.value.valueInt64;
    }
}

static inline bool
t()
{
    // std::cout << "---";
    return true;
}
static inline bool
f()
{
    // std::cout << "nope\t";
    return false;
}

// implement your algorithm in this function
static inline bool
IntegerOverflow(Instruction *instr, uint64_t flagsValue)
{
    OperatorType ot = instr->getOperatorType();
    switch (ot) {
    case kOPadd: {
        // if (flagsValue & 0x0080)
        //     std::cout << ".";
        // else
        //     std::cout << " ";
        // if (flagsValue & 0x0001)
        //     std::cout << ".";
        // else
        //     std::cout << " ";
        Operand a = instr->getSrcOperand(0);
        int64_t a_value = GetIntValue(a);
        Operand b = instr->getSrcOperand(1);
        int64_t b_value = GetIntValue(b);
        Operand res = instr->getDstOperand(0);

        int64_t max = GetMaxValue(res);
        int64_t min = GetMinValue(res);

        if (a_value > 0 && b_value > max - a_value)
            return t();
        else if (a_value < 0 && b_value < min - a_value)
            return t();
        else
            return f();
    }
    case kOPsub: {
        // if (flagsValue & 0x0080)
        //     std::cout << ".";
        // else
        //     std::cout << " ";
        // if (flagsValue & 0x0001)
        //     std::cout << ".";
        // else
        //     std::cout << " ";

        Operand a = instr->getSrcOperand(0);
        int64_t a_value = GetIntValue(a);
        Operand b = instr->getSrcOperand(1);
        int64_t b_value = GetIntValue(b);
        Operand res = instr->getDstOperand(0);

        int64_t max = GetMaxValue(res);
        int64_t min = GetMinValue(res);

        if (a_value < 0 && b_value > -(min - a_value))
            return t();
        else if (a_value > 0 && b_value < -(max - a_value))
            return t();
        else
            return f();
    }
    case kOPshl: {
        // if (flagsValue & 0x0080)
        //     std::cout << ".";
        // else
        //     std::cout << " ";
        // if (flagsValue & 0x0001)
        //     std::cout << ".";
        // else
        //     std::cout << " ";

        Operand a = instr->getSrcOperand(0);
        int64_t a_value = GetIntValue(a);
        Operand b = instr->getSrcOperand(1);
        int64_t b_value = GetIntValue(b);
        Operand res = instr->getDstOperand(0);

        int64_t mask;
        switch (res.size) {
        case 1: mask = 0x80; break;
        case 2: mask = 0x8000; break;
        case 4: mask = 0x80000000; break;
        default: mask = 0x8000000000000000; break;
        }
        if ((a_value & mask) != ((a_value << b_value) & mask))
            return t();
        else
            return f();
    }
    default: return f();
    }
}

void
OnAfterInsExec(Instruction *instr, context_handle_t contxt, uint64_t flagsValue,
               CtxtContainer *ctxtContainer)
{
    if (IntegerOverflow(instr, flagsValue)) {
        ctxtContainer->addCtxt(contxt);
    }
    // add: Destination = Source0 + Source1
    if (instr->getOperatorType() == OperatorType::kOPadd) {
        Operand srcOpnd0 = instr->getSrcOperand(0);
        Operand srcOpnd1 = instr->getSrcOperand(1);
        Operand dstOpnd = instr->getDstOperand(0);
        std::bitset<64> bitFlagsValue(flagsValue);

        cout << "ip(" << hex << instr->ip << "):"
             << "add " << dec << GetOpndIntValue(srcOpnd0) << " "
             << GetOpndIntValue(srcOpnd1) << " -> " << GetOpndIntValue(dstOpnd) << " "
             << bitFlagsValue << endl;
    }
    // sub: Destination = Source0 - Source1
    if (instr->getOperatorType() == OperatorType::kOPsub) {
        Operand srcOpnd0 = instr->getSrcOperand(0);
        Operand srcOpnd1 = instr->getSrcOperand(1);
        Operand dstOpnd = instr->getDstOperand(0);

        std::bitset<64> bitFlagsValue(flagsValue);

        cout << "ip(" << hex << instr->ip << "):"
             << "sub " << dec << GetOpndIntValue(srcOpnd0) << " "
             << GetOpndIntValue(srcOpnd1) << " -> " << GetOpndIntValue(dstOpnd) << " "
             << bitFlagsValue << endl;
    }
    // shl: Destination = Source0 << Source1
    if (instr->getOperatorType() == OperatorType::kOPshl) {
        Operand srcOpnd0 = instr->getSrcOperand(0);
        Operand srcOpnd1 = instr->getSrcOperand(1);
        Operand dstOpnd = instr->getDstOperand(0);
        std::bitset<64> bitFlagsValue(flagsValue);

        cout << "ip(" << hex << instr->ip << "):"
             << "shl " << dec << GetOpndIntValue(srcOpnd0) << " "
             << GetOpndIntValue(srcOpnd1) << " -> " << GetOpndIntValue(dstOpnd) << " "
             << bitFlagsValue << endl;
    }
}

void
OnBeforeAppExit(CtxtContainer *ctxtContainer)
{
    Profile::profile_t *profile = new Profile::profile_t();
    profile->add_metric_type(1, "", "integer overflow occurrence");

    std::vector<context_handle_t> list = ctxtContainer->getCtxtList();
    for (size_t i = 0; i < list.size(); i++) {
        inner_context_t *cur_ctxt = drcctlib_get_full_cct(list[i]);
        profile->add_sample(cur_ctxt)->append_metirc((uint64_t)1);
        drcctlib_free_full_cct(cur_ctxt);
    }
    profile->serialize_to_file("integer-overflow-profile.drcctprof");
    delete profile;

    file_t profileTxt = dr_open_file("integer-overflow-profile.txt",
                                     DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
    DR_ASSERT(profileTxt != INVALID_FILE);
    for (size_t i = 0; i < list.size(); i++) {
        dr_fprintf(profileTxt, "INTEGER OVERFLOW\n");
        drcctlib_print_backtrace_first_item(profileTxt, list[i], true, false);
        dr_fprintf(profileTxt, "=>BACKTRACE\n");
        drcctlib_print_backtrace(profileTxt, list[i], false, true, -1);
        dr_fprintf(profileTxt, "\n\n");
    }
    dr_close_file(profileTxt);
}

}