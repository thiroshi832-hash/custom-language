#pragma once
#include <QString>

enum class Opcode {
    // Stack operations
    PUSH_CONST,       // vOperand: constant value (QVariant)
    PUSH_NULL,
    POP,
    DUP,

    // Variables
    LOAD_VAR,         // sOperand: variable name
    STORE_VAR,        // sOperand: variable name
    DECLARE_VAR,      // sOperand: variable name

    // Arithmetic
    ADD, SUB, MUL, DIV, INT_DIV, MOD, NEG, POW,

    // String
    CONCAT,

    // Comparison
    CMP_EQ, CMP_NEQ, CMP_LT, CMP_GT, CMP_LTE, CMP_GTE,

    // Logical
    LOG_AND, LOG_OR, LOG_NOT,

    // Control flow
    JUMP,             // vOperand: target index (int)
    JUMP_IF_FALSE,    // vOperand: target index (int)
    JUMP_IF_TRUE,     // vOperand: target index (int)

    // Calls
    CALL,             // sOperand: name, vOperand: argc (int)
    CALL_BUILTIN,     // sOperand: name, vOperand: argc (int)
    RETURN,           // vOperand: 1 = has value (Function), 0 = no value (Sub)

    // Misc
    HALT,
    NOP
};

inline QString opcodeName(Opcode op) {
    switch (op) {
    case Opcode::PUSH_CONST:   return "PUSH_CONST";
    case Opcode::PUSH_NULL:    return "PUSH_NULL";
    case Opcode::POP:          return "POP";
    case Opcode::DUP:          return "DUP";
    case Opcode::LOAD_VAR:     return "LOAD_VAR";
    case Opcode::STORE_VAR:    return "STORE_VAR";
    case Opcode::DECLARE_VAR:  return "DECLARE_VAR";
    case Opcode::ADD:          return "ADD";
    case Opcode::SUB:          return "SUB";
    case Opcode::MUL:          return "MUL";
    case Opcode::DIV:          return "DIV";
    case Opcode::INT_DIV:      return "INT_DIV";
    case Opcode::MOD:          return "MOD";
    case Opcode::NEG:          return "NEG";
    case Opcode::POW:          return "POW";
    case Opcode::CONCAT:       return "CONCAT";
    case Opcode::CMP_EQ:       return "CMP_EQ";
    case Opcode::CMP_NEQ:      return "CMP_NEQ";
    case Opcode::CMP_LT:       return "CMP_LT";
    case Opcode::CMP_GT:       return "CMP_GT";
    case Opcode::CMP_LTE:      return "CMP_LTE";
    case Opcode::CMP_GTE:      return "CMP_GTE";
    case Opcode::LOG_AND:      return "LOG_AND";
    case Opcode::LOG_OR:       return "LOG_OR";
    case Opcode::LOG_NOT:      return "LOG_NOT";
    case Opcode::JUMP:         return "JUMP";
    case Opcode::JUMP_IF_FALSE:return "JUMP_IF_FALSE";
    case Opcode::JUMP_IF_TRUE: return "JUMP_IF_TRUE";
    case Opcode::CALL:         return "CALL";
    case Opcode::CALL_BUILTIN: return "CALL_BUILTIN";
    case Opcode::RETURN:       return "RETURN";
    case Opcode::HALT:         return "HALT";
    case Opcode::NOP:          return "NOP";
    default:                   return "???";
    }
}
