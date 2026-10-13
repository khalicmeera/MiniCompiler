#include "ir.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

bool parseInteger(const std::string& text, long long& value) {
    if (text.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const long long parsed = std::strtoll(text.c_str(), &end, 10);
    if (errno == ERANGE || end == text.c_str() || *end != '\0') return false;
    value = parsed;
    return true;
}

bool checkedEvaluate(char op, long long a, long long b, long long& result) {
    switch (op) {
        case '+':
            if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b)) return false;
            result = a + b; return true;
        case '-':
            if ((b < 0 && a > LLONG_MAX + b) || (b > 0 && a < LLONG_MIN + b)) return false;
            result = a - b; return true;
        case '*': {
            if (a == 0 || b == 0) { result = 0; return true; }
            if ((a == -1 && b == LLONG_MIN) || (b == -1 && a == LLONG_MIN)) return false;
            if (a > 0) {
                if (b > 0 && a > LLONG_MAX / b) return false;
                if (b < 0 && b < LLONG_MIN / a) return false;
            } else {
                if (b > 0 && a < LLONG_MIN / b) return false;
                if (b < 0 && a < LLONG_MAX / b) return false;
            }
            result = a * b; return true;
        }
        case '/':
            if (b == 0 || (a == LLONG_MIN && b == -1)) return false;
            result = a / b; return true;
        default: return false;
    }
}

std::string lowerExpr(const Expr* expr, std::vector<IRInstruction>& out, int& tempCounter) {
    if (const auto* number = dynamic_cast<const NumberExpr*>(expr)) return std::to_string(number->value);
    if (const auto* variable = dynamic_cast<const VariableExpr*>(expr)) return variable->name;
    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expr)) {
        const std::string lhs = lowerExpr(binary->left.get(), out, tempCounter);
        const std::string rhs = lowerExpr(binary->right.get(), out, tempCounter);
        const std::string temp = "t" + std::to_string(++tempCounter);
        out.push_back({IRKind::Binary, temp, lhs, binary->op, rhs});
        return temp;
    }
    throw std::runtime_error("IR generation encountered an unknown expression.");
}

std::string substitute(const std::string& operand,
                       const std::unordered_map<std::string, long long>& constants) {
    const auto it = constants.find(operand);
    return it == constants.end() ? operand : std::to_string(it->second);
}

bool isTemporary(const std::string& name) {
    if (name.size() < 2 || name[0] != 't') return false;
    for (std::size_t i = 1; i < name.size(); ++i)
        if (name[i] < '0' || name[i] > '9') return false;
    return true;
}

bool uses(const IRInstruction& ins, const std::string& name) {
    if (ins.kind == IRKind::Binary) return ins.left == name || ins.right == name;
    return ins.left == name; // Assign source or Print value
}

void eliminateDeadTemporaries(std::vector<IRInstruction>& instructions) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t i = instructions.size(); i > 0; --i) {
            const std::size_t index = i - 1;
            const auto& ins = instructions[index];
            if (ins.kind != IRKind::Assign || !isTemporary(ins.result)) continue;
            bool used = false;
            for (std::size_t j = 0; j < instructions.size(); ++j) {
                if (j != index && uses(instructions[j], ins.result)) { used = true; break; }
            }
            if (!used) {
                instructions.erase(instructions.begin() + static_cast<std::ptrdiff_t>(index));
                changed = true;
                break;
            }
        }
    }
}

} // namespace

std::vector<IRInstruction> generateIR(const Program& program) {
    std::vector<IRInstruction> instructions;
    int tempCounter = 0;
    for (const auto& stmt : program) {
        if (const auto* let = dynamic_cast<const LetStmt*>(stmt.get())) {
            const std::string value = lowerExpr(let->value.get(), instructions, tempCounter);
            instructions.push_back({IRKind::Assign, let->name, value, '\0', ""});
        } else if (const auto* print = dynamic_cast<const PrintStmt*>(stmt.get())) {
            const std::string value = lowerExpr(print->value.get(), instructions, tempCounter);
            instructions.push_back({IRKind::Print, "", value, '\0', ""});
        } else {
            throw std::runtime_error("IR generation encountered an unknown statement.");
        }
    }
    return instructions;
}

void optimizeIR(std::vector<IRInstruction>& instructions) {
    std::unordered_map<std::string, long long> constants;
    for (auto& ins : instructions) {
        if (ins.kind == IRKind::Binary) {
            ins.left = substitute(ins.left, constants);
            ins.right = substitute(ins.right, constants);
            long long lhs = 0, rhs = 0, result = 0;
            if (parseInteger(ins.left, lhs) && parseInteger(ins.right, rhs) &&
                checkedEvaluate(ins.op, lhs, rhs, result)) {
                ins.kind = IRKind::Assign;
                ins.left = std::to_string(result);
                ins.right.clear();
                ins.op = '\0';
                constants[ins.result] = result;
            } else {
                constants.erase(ins.result);
            }
        } else if (ins.kind == IRKind::Assign) {
            ins.left = substitute(ins.left, constants);
            long long value = 0;
            if (parseInteger(ins.left, value)) constants[ins.result] = value;
            else constants.erase(ins.result);
        } else if (ins.kind == IRKind::Print) {
            ins.left = substitute(ins.left, constants);
        }
    }
    eliminateDeadTemporaries(instructions);
}

void writeIR(const std::vector<IRInstruction>& instructions, std::ostream& out) {
    for (const auto& ins : instructions) {
        switch (ins.kind) {
            case IRKind::Binary:
                out << ins.result << " = " << ins.left << ' ' << ins.op << ' ' << ins.right << '\n'; break;
            case IRKind::Assign:
                out << ins.result << " = " << ins.left << '\n'; break;
            case IRKind::Print:
                out << "print " << ins.left << '\n'; break;
        }
    }
}
