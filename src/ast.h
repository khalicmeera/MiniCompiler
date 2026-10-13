#ifndef MINICOMPILER_AST_H
#define MINICOMPILER_AST_H

#include <memory>
#include <string>
#include <vector>

struct Expr {
    virtual ~Expr() = default;
};

struct NumberExpr final : Expr {
    explicit NumberExpr(long long v) : value(v) {}
    long long value;
};

struct VariableExpr final : Expr {
    explicit VariableExpr(std::string n) : name(std::move(n)) {}
    std::string name;
};

struct BinaryExpr final : Expr {
    BinaryExpr(char operation, std::unique_ptr<Expr> lhs,
               std::unique_ptr<Expr> rhs)
        : op(operation), left(std::move(lhs)), right(std::move(rhs)) {}
    char op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct Stmt {
    virtual ~Stmt() = default;
};

struct LetStmt final : Stmt {
    LetStmt(std::string n, std::unique_ptr<Expr> v)
        : name(std::move(n)), value(std::move(v)) {}
    std::string name;
    std::unique_ptr<Expr> value;
};

struct PrintStmt final : Stmt {
    explicit PrintStmt(std::unique_ptr<Expr> v) : value(std::move(v)) {}
    std::unique_ptr<Expr> value;
};

using Program = std::vector<std::unique_ptr<Stmt>>;

#endif
