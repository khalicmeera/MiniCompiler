#include "ast.h"
#include "ir.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// -------------------- Lexer --------------------
enum class TokenKind { Identifier, Number, Let, Print, Plus, Minus, Star, Slash,
                        Equal, Semicolon, LeftParen, RightParen, End };
struct Token { TokenKind kind; std::string text; std::size_t position; };

class Lexer {
public:
    explicit Lexer(std::string source) : source_(std::move(source)) {}
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (position_ < source_.size()) {
            const char c = source_[position_];
            if (std::isspace(static_cast<unsigned char>(c))) { ++position_; continue; }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                const std::size_t start = position_++;
                while (position_ < source_.size()) {
                    const char next = source_[position_];
                    if (!std::isalnum(static_cast<unsigned char>(next)) && next != '_') break;
                    ++position_;
                }
                std::string word = source_.substr(start, position_ - start);
                if (word == "let") tokens.push_back({TokenKind::Let, word, start});
                else if (word == "print") tokens.push_back({TokenKind::Print, word, start});
                else tokens.push_back({TokenKind::Identifier, word, start});
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(c))) {
                const std::size_t start = position_++;
                while (position_ < source_.size() &&
                       std::isdigit(static_cast<unsigned char>(source_[position_]))) ++position_;
                tokens.push_back({TokenKind::Number, source_.substr(start, position_ - start), start});
                continue;
            }
            TokenKind kind;
            switch (c) {
                case '+': kind = TokenKind::Plus; break;
                case '-': kind = TokenKind::Minus; break;
                case '*': kind = TokenKind::Star; break;
                case '/': kind = TokenKind::Slash; break;
                case '=': kind = TokenKind::Equal; break;
                case ';': kind = TokenKind::Semicolon; break;
                case '(': kind = TokenKind::LeftParen; break;
                case ')': kind = TokenKind::RightParen; break;
                default:
                    throw std::runtime_error("Lexer error at position " + std::to_string(position_) +
                                             ": unexpected character '" + std::string(1, c) + "'");
            }
            tokens.push_back({kind, std::string(1, c), position_++});
        }
        tokens.push_back({TokenKind::End, "", position_});
        return tokens;
    }
private:
    std::string source_;
    std::size_t position_ = 0;
};

// -------------------- Parser --------------------
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}
    Program parseProgram() {
        Program program;
        while (!check(TokenKind::End)) program.push_back(parseStatement());
        return program;
    }
private:
    const std::vector<Token>& tokens_;
    std::size_t current_ = 0;
    const Token& peek() const { return tokens_.at(current_); }
    const Token& previous() const { return tokens_.at(current_ - 1); }
    bool check(TokenKind kind) const { return peek().kind == kind; }
    bool match(TokenKind kind) { if (!check(kind)) return false; ++current_; return true; }
    const Token& consume(TokenKind kind, const std::string& message) {
        if (check(kind)) return tokens_.at(current_++);
        throw std::runtime_error("Parser error at position " + std::to_string(peek().position) + ": " + message);
    }
    std::unique_ptr<Stmt> parseStatement() {
        if (match(TokenKind::Let)) {
            const Token& name = consume(TokenKind::Identifier, "expected variable name after 'let'");
            consume(TokenKind::Equal, "expected '=' after variable name");
            auto value = parseExpression();
            consume(TokenKind::Semicolon, "expected ';' after declaration");
            return std::make_unique<LetStmt>(name.text, std::move(value));
        }
        if (match(TokenKind::Print)) {
            auto value = parseExpression();
            consume(TokenKind::Semicolon, "expected ';' after print expression");
            return std::make_unique<PrintStmt>(std::move(value));
        }
        throw std::runtime_error("Parser error at position " + std::to_string(peek().position) +
                                 ": expected 'let' or 'print'");
    }
    std::unique_ptr<Expr> parseExpression() { return parseAdditive(); }
    std::unique_ptr<Expr> parseAdditive() {
        auto expr = parseMultiplicative();
        while (match(TokenKind::Plus) || match(TokenKind::Minus)) {
            const char op = previous().text[0];
            auto rhs = parseMultiplicative();
            expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(rhs));
        }
        return expr;
    }
    std::unique_ptr<Expr> parseMultiplicative() {
        auto expr = parsePrimary();
        while (match(TokenKind::Star) || match(TokenKind::Slash)) {
            const char op = previous().text[0];
            auto rhs = parsePrimary();
            expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(rhs));
        }
        return expr;
    }
    std::unique_ptr<Expr> parsePrimary() {
        if (match(TokenKind::Number)) {
            const Token& token = previous();
            try { return std::make_unique<NumberExpr>(std::stoll(token.text)); }
            catch (const std::out_of_range&) {
                throw std::runtime_error("Parser error: integer literal is out of range");
            }
        }
        if (match(TokenKind::Identifier)) return std::make_unique<VariableExpr>(previous().text);
        if (match(TokenKind::LeftParen)) {
            auto expr = parseExpression();
            consume(TokenKind::RightParen, "expected ')' after expression");
            return expr;
        }
        throw std::runtime_error("Parser error at position " + std::to_string(peek().position) +
                                 ": expected a number, variable, or '('");
    }
};

// -------------------- Semantic analysis --------------------
void checkExpr(const Expr* expr, const std::unordered_set<std::string>& declared) {
    if (const auto* variable = dynamic_cast<const VariableExpr*>(expr)) {
        if (declared.find(variable->name) == declared.end())
            throw std::runtime_error("Semantic error: use of undeclared variable '" + variable->name + "'");
    } else if (const auto* binary = dynamic_cast<const BinaryExpr*>(expr)) {
        checkExpr(binary->left.get(), declared);
        checkExpr(binary->right.get(), declared);
    }
}

void semanticCheck(const Program& program) {
    std::unordered_set<std::string> declared;
    for (const auto& stmt : program) {
        if (const auto* let = dynamic_cast<const LetStmt*>(stmt.get())) {
            if (declared.find(let->name) != declared.end())
                throw std::runtime_error("Semantic error: duplicate declaration of variable '" + let->name + "'");
            checkExpr(let->value.get(), declared);
            declared.insert(let->name);
        } else if (const auto* print = dynamic_cast<const PrintStmt*>(stmt.get())) {
            checkExpr(print->value.get(), declared);
        }
    }
}

// -------------------- Safe constant evaluation and division validation --------------------
bool evaluateConstant(char op, long long lhs, long long rhs, long long& result) {
    switch (op) {
        case '+':
            if ((rhs > 0 && lhs > LLONG_MAX - rhs) || (rhs < 0 && lhs < LLONG_MIN - rhs)) return false;
            result = lhs + rhs; return true;
        case '-':
            if ((rhs < 0 && lhs > LLONG_MAX + rhs) || (rhs > 0 && lhs < LLONG_MIN + rhs)) return false;
            result = lhs - rhs; return true;
        case '*': {
            if (lhs == 0 || rhs == 0) { result = 0; return true; }
            if ((lhs == -1 && rhs == LLONG_MIN) || (rhs == -1 && lhs == LLONG_MIN)) return false;
            if (lhs > 0) {
                if (rhs > 0 && lhs > LLONG_MAX / rhs) return false;
                if (rhs < 0 && rhs < LLONG_MIN / lhs) return false;
            } else {
                if (rhs > 0 && lhs < LLONG_MIN / rhs) return false;
                if (rhs < 0 && lhs < LLONG_MAX / rhs) return false;
            }
            result = lhs * rhs; return true;
        }
        case '/':
            if (rhs == 0 || (lhs == LLONG_MIN && rhs == -1)) return false;
            result = lhs / rhs; return true;
        default: return false;
    }
}

bool constantValue(const Expr* expr,
                   const std::unordered_map<std::string, long long>& constants,
                   long long& result) {
    if (const auto* number = dynamic_cast<const NumberExpr*>(expr)) {
        result = number->value; return true;
    }
    if (const auto* variable = dynamic_cast<const VariableExpr*>(expr)) {
        const auto it = constants.find(variable->name);
        if (it == constants.end()) return false;
        result = it->second; return true;
    }
    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expr)) {
        long long lhs = 0, rhs = 0;
        return constantValue(binary->left.get(), constants, lhs) &&
               constantValue(binary->right.get(), constants, rhs) &&
               evaluateConstant(binary->op, lhs, rhs, result);
    }
    return false;
}

void checkDivisionExpr(const Expr* expr,
                       const std::unordered_map<std::string, long long>& constants) {
    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expr)) {
        checkDivisionExpr(binary->left.get(), constants);
        checkDivisionExpr(binary->right.get(), constants);
        if (binary->op == '/') {
            long long divisor = 0;
            if (constantValue(binary->right.get(), constants, divisor) && divisor == 0)
                throw std::runtime_error("Compile-time error: division by zero");
        }
    }
}

void validateDivisionByZero(const Program& program) {
    std::unordered_map<std::string, long long> constants;
    for (const auto& stmt : program) {
        if (const auto* let = dynamic_cast<const LetStmt*>(stmt.get())) {
            checkDivisionExpr(let->value.get(), constants);
            long long value = 0;
            if (constantValue(let->value.get(), constants, value)) constants[let->name] = value;
            else constants.erase(let->name);
        } else if (const auto* print = dynamic_cast<const PrintStmt*>(stmt.get())) {
            checkDivisionExpr(print->value.get(), constants);
        }
    }
}

// -------------------- AST constant folding --------------------
std::unique_ptr<Expr> optimizeExpr(std::unique_ptr<Expr> expr) {
    auto* binary = dynamic_cast<BinaryExpr*>(expr.get());
    if (!binary) return expr;
    binary->left = optimizeExpr(std::move(binary->left));
    binary->right = optimizeExpr(std::move(binary->right));
    auto* lhs = dynamic_cast<NumberExpr*>(binary->left.get());
    auto* rhs = dynamic_cast<NumberExpr*>(binary->right.get());
    if (lhs && rhs) {
        long long result = 0;
        if (evaluateConstant(binary->op, lhs->value, rhs->value, result))
            return std::make_unique<NumberExpr>(result);
    }
    return expr;
}
void optimizeAST(Program& program) {
    for (auto& stmt : program) {
        if (auto* let = dynamic_cast<LetStmt*>(stmt.get())) let->value = optimizeExpr(std::move(let->value));
        else if (auto* print = dynamic_cast<PrintStmt*>(stmt.get())) print->value = optimizeExpr(std::move(print->value));
    }
}

// -------------------- C code generation --------------------
std::string cExpr(const Expr* expr) {
    if (const auto* number = dynamic_cast<const NumberExpr*>(expr)) return std::to_string(number->value);
    if (const auto* variable = dynamic_cast<const VariableExpr*>(expr)) return variable->name;
    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expr))
        return "(" + cExpr(binary->left.get()) + " " + binary->op + " " + cExpr(binary->right.get()) + ")";
    throw std::runtime_error("C generation encountered an unknown expression.");
}
void generateC(const Program& program, std::ostream& out) {
    out << "#include <stdio.h>\n\nint main(void) {\n";
    for (const auto& stmt : program) {
        if (const auto* let = dynamic_cast<const LetStmt*>(stmt.get()))
            out << "    long long " << let->name << " = " << cExpr(let->value.get()) << ";\n";
        else if (const auto* print = dynamic_cast<const PrintStmt*>(stmt.get()))
            out << "    printf(\"%lld\\n\", (long long)(" << cExpr(print->value.get()) << "));\n";
    }
    out << "    return 0;\n}\n";
}

std::string replaceExtension(const std::string& filename, const std::string& extension) {
    const std::size_t dot = filename.find_last_of('.');
    const std::size_t slash = filename.find_last_of("/\\");
    if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
        return filename.substr(0, dot) + extension;
    return filename + extension;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <input.mc> [output.c]\n";
        return 1;
    }
    const std::string inputFilename = argv[1];
    const std::string outputFilename = argc == 3 ? argv[2] : "output.c";
    const std::string irFilename = replaceExtension(outputFilename, ".ir");
    const std::string unoptimizedIRFilename = replaceExtension(outputFilename, ".unoptimized.ir");

    try {
        std::ifstream input(inputFilename);
        if (!input) throw std::runtime_error("Could not open input file '" + inputFilename + "'");
        const std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        Lexer lexer(source);
        const auto tokens = lexer.tokenize();
        Parser parser(tokens);
        Program program = parser.parseProgram();

        semanticCheck(program);
        validateDivisionByZero(program); // Validate before creating output files.

        const auto unoptimizedIR = generateIR(program);
        auto optimizedIR = unoptimizedIR;
        optimizeIR(optimizedIR);

        std::ofstream rawIROutput(unoptimizedIRFilename);
        if (!rawIROutput) throw std::runtime_error("Could not create IR output file '" + unoptimizedIRFilename + "'");
        writeIR(unoptimizedIR, rawIROutput);
        rawIROutput.close();

        std::ofstream irOutput(irFilename);
        if (!irOutput) throw std::runtime_error("Could not create IR output file '" + irFilename + "'");
        writeIR(optimizedIR, irOutput);
        irOutput.close();

        optimizeAST(program);
        std::ofstream cOutput(outputFilename);
        if (!cOutput) throw std::runtime_error("Could not create C output file '" + outputFilename + "'");
        generateC(program, cOutput);
        cOutput.close();

        std::cout << "Compilation successful.\n"
                  << "C output: " << outputFilename << "\n"
                  << "Unoptimized IR output: " << unoptimizedIRFilename << "\n"
                  << "Optimized IR output: " << irFilename << "\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
