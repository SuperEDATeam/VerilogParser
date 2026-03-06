#pragma once
#ifndef MYPARSER_H
#define MYPARSER_H
//#include "Mynetlist.h"
#include <iostream>
#include "MyMidData.h"

using namespace std;

// 定义词法单元的类型
enum class TokenType {
    Identifier,
    Keyword,
    Symbol,
    EndOfInput
};

// 词法单元类
class Token {
public:
    TokenType type;
    string value;

    Token(TokenType type, string value) : type(type), value(move(value)) {}
};

// 词法分析器
class Lexer {
public:
    explicit Lexer(const string& input) : input(input), position(0) {}
    Token NextToken();
    string GetWholeExpression();
    void init() {
        this->position = 0;
    }
    //Token GetTokenFromSubExpression(string str);
private:
    const string input;
    size_t position;
    unordered_set<string> keywords = { "module", "input", "output", "wire", "assign", "endmodule" };
};

// 语法分析器
class Parser {
public:
    explicit Parser(Lexer& lexer) : lexer(lexer) {}
    void Parse(MyMidModuleMap& listmid);
    void  ParseStatement(MyMidModule* pmod, Token token,vector<PGate*>& all_gates);
    void Debug() const {
        std::cout << "Parser Object:\n";
        std::cout << "  Lexer reference: " << &lexer << "\n";
        // 打印其他成员变量（如果有）
    }
private:
    Lexer& lexer;
    vector<PGate*> all_gates;
};



#endif