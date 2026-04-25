#include "opcodestream.h"

QString OpcodeStream::disassemble() const {
    QString out;
    // Print function table
    if (!m_funcTable.isEmpty()) {
        out += "=== Function Table ===\n";
        for (auto it = m_funcTable.begin(); it != m_funcTable.end(); ++it) {
            const FuncInfo &fi = it.value();
            out += QString("  %1 %2(%3) @ %4\n")
                   .arg(fi.isFunction ? "Function" : "Sub")
                   .arg(fi.name)
                   .arg(fi.params.join(", "))
                   .arg(fi.startAddress);
        }
        out += "\n";
    }

    out += "=== Instructions ===\n";
    for (int i = 0; i < m_instructions.size(); ++i) {
        const Instruction &ins = m_instructions[i];
        QString line = QString("%1: %2")
                       .arg(i, 4)
                       .arg(opcodeName(ins.op), -16);

        if (!ins.sOperand.isEmpty())
            line += " " + ins.sOperand;

        if (!ins.vOperand.isNull()) {
            if (!ins.sOperand.isEmpty()) line += ",";
            line += " " + ins.vOperand.toString();
        }

        if (ins.line > 0)
            line += QString("   ; line %1").arg(ins.line);

        out += line + "\n";
    }
    return out;
}
