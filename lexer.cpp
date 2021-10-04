#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

//===----------------------------------===//
// Lexer
// Lexter returns tokens [0~255]
//===----------------------------------===//

enum Token
{
    tok_eof = -1,

    //command
    tok_def = -2,
    tok_extern = -3,

    //primary
    tok_identifier = -4,
    tok_number = -5
};

static std::string IdentifierStr; // Filled in if tok_indetifier
static double NumVal;             // Filled in if tok_number

static int gettok()
{
    static int LastChar = ' ';

    while (isspace(LastChar))
        LastChar = getchar();

    if (isalpha(LastChar))
    {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;

        if (IdentifierStr == "def")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.')
    { // Number: [0-9.]+
        std::string NumStr;

        do
        {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#')
    { // Comment
        do
            LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }

    if (LastChar == EOF)
        return tok_eof;

    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

//===----------------------------------===//
// AST
//===----------------------------------===//

namespace
{
    /// ExprAST - Base class for all expression nodes
    class ExprAST
    {
    public:
        virtual ~ExprAST() = default;
    };

    // NumberExprAST - "1.0"
    class NumberExprAST : public ExprAST
    {
        double Val;

    public:
        NumberExprAST(double Val) : Val(Val) {}
    };

    // VariableExprAST - "a"
    // TODO: Warning
    class VariableExprAST : public ExprAST
    {
        std::string Name;

    public:
        VariableExprAST(const std::string &Name) : Name(Name) {}
    };

    // BinaryExprAST - expression class for a binary operator
    class BinaryExprAST : public ExprAST
    {
        char Op;
        std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                      std::unique_ptr<ExprAST> RHS)
            : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    };

    // CallExprAST - expression class for function calls
    class CallExprAST : public ExprAST
    {
        std::string Callee;
        std::vector<std::unique_ptr<ExprAST>> Args;

    public:
        CallExprAST(const std::string &Callee,
                    std::vector<std::unique_ptr<ExprAST>> Args)
            : Callee(Callee), Args(std::move(Args)) {}
    };

    // PrototypeAST : The prototype for a function
    class PrototypeAST
    {
        std::string Name;
        std::vector<std::string> Args;

    public:
        PrototypeAST(const std::string &Name, std::vector<std::string> Args)
            : Name(Name), Args(std::move(Args)) {}
        const std::string &getName() const { return Name; }
    };

    // FunctionAST
    class FunctionAST {
        std::unique_ptr<PrototypeAST> Proto;
        std::unique_ptr<ExprAST> Body;

        public:
            FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                        std::unique_ptr<ExprAST> Body)
                    : Proto(std::move(Proto)), Body(std::move(Body)) {}
    };

} // namespace


//===----------------------------------===//
// Parser
//===----------------------------------===//

static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

static std::map<char, int> BinopPrecedence;