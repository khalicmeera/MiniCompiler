#ifndef MINICOMPILER_IR_H
#define MINICOMPILER_IR_H

#include "ast.h"
#include <iosfwd>
#include <string>
#include <vector>

enum class IRKind { Binary, Assign, Print };

struct IRInstruction {
    IRKind kind;
    std::string result;
    std::string left;
    char op = '\0';
    std::string right;
};

std::vector<IRInstruction> generateIR(const Program& program);
void optimizeIR(std::vector<IRInstruction>& instructions);
void writeIR(const std::vector<IRInstruction>& instructions, std::ostream& out);

#endif
