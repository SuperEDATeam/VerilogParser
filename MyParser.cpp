//#include "Mynetlist.h"
#include "MyParser.h"
#include "utils.h"
#include <assert.h> 
#include <stdexcept>


Token Lexer::NextToken() {
    while (isspace(input[position])) {
        ++position;
    }

    if (position >= input.length()) {
        return Token(TokenType::EndOfInput, "");
    }

    char currentChar = input[position++];

    if (isalpha(currentChar)) { // Identifier or Keyword
        string identifier;
        while (isalnum(currentChar) || currentChar == '_') {
            identifier += currentChar;
            if (position < input.length()) {
                currentChar = input[position++];
            }
            else {
                break;
            }
        }
        position--; // Rollback the last character read
        if (keywords.count(identifier)) {
            return Token(TokenType::Keyword, identifier);
        }
        else {
            return Token(TokenType::Identifier, identifier);
        }
    }
    else if (currentChar == ',' || currentChar == ';' || currentChar == '=' || currentChar == '&' || currentChar == '|' || currentChar == '(' || currentChar == ')') {
        return Token(TokenType::Symbol, string(1, currentChar));
    }

    return Token(TokenType::EndOfInput, ""); // Unknown token
}

string Lexer::GetWholeExpression()
{
    string exp;
    while (isspace(input[position])) {
        ++position;
    }
    auto startpos = position;
    while (input[position] != ';')
    {
        position++;
    }
    exp = input.substr(startpos, position - startpos);

    return input.substr(startpos, position - startpos);
}


void  Parser::Parse(MyMidModuleMap& midmap) {
    auto moduleToken = lexer.NextToken();
    if (moduleToken.type != TokenType::Keyword || moduleToken.value != "module") {
        throw runtime_error("Expected 'module'");
    }
    auto moduleNameToken = lexer.NextToken();
    if (moduleNameToken.type != TokenType::Identifier) {
        throw runtime_error("Expected module name");
    }
    string moduleName = moduleNameToken.value;
    //m_AstRoot = make_unique<ModuleNode>(moduleName);

    MyMidModule* pmod = new MyMidModule;
    pmod->modname = moduleName;
    midmap.m_module_map[moduleName] = pmod;
    // Parse parameter
    auto token = lexer.NextToken();
    while (token.value != ";")
    {
        if (token.type == TokenType::Identifier)
        {
            PWire* param = new PWire;
            param->wiretype = PWire::INPORT;
            param->name = token.value;
            pmod->wires[token.value] = param;
        }
        token = lexer.NextToken();
    }
    // Parse input  port
    token = lexer.NextToken();
    if (token.type == TokenType::Keyword && token.value == "input")
        token = lexer.NextToken();
        while (token.value != ";")
        {  
        if(token.type == TokenType::Identifier)
        { if (pmod->is_in_wires(token.value))
            pmod->wires[token.value]->wiretype = PWire::INPORT;
        else
            throw runtime_error("Undefined inport");
        }
        token = lexer.NextToken();
        }
     // Parse output port
     token = lexer.NextToken();
     if (token.type == TokenType::Keyword && token.value == "output")
            token = lexer.NextToken();
     while (token.value != ";")
       {
         if (token.type == TokenType::Identifier)
         {
             if (pmod->is_in_wires(token.value))
                 pmod->wires[token.value]->wiretype = PWire::OUTPORT;
             else
                 throw runtime_error("Undefined outport");
         }
            token = lexer.NextToken();
       }
     // Parse Declaration
     token = lexer.NextToken();
     if (token.type == TokenType::Keyword && token.value == "wire") {
         while (true) {
             auto wireToken = lexer.NextToken();
             if (wireToken.type != TokenType::Identifier) {
                 throw runtime_error("Expected wire name");
             }
             PWire* param = new PWire;
             param->wiretype = PWire::MIDDLE;
             param->name = wireToken.value;
             pmod->wires[wireToken.value] = param;

             auto next = lexer.NextToken(); // 看下一个 token 是什么
             if (next.type == TokenType::Symbol && next.value == ",") {
                 continue;
             }
             else if (next.type == TokenType::Symbol && next.value == ";") {
                 break;
             }
             else {
                 throw runtime_error("Expected ',' or ';' after wire name");
             }
         }
     }
    // Parse statements
    token = lexer.NextToken();
    while (token.type != TokenType::Keyword || token.value != "endmodule") {
        ParseStatement(pmod, token, all_gates);
        token = lexer.NextToken();
    }
}

void  Parser::ParseStatement(MyMidModule* pmod,Token token,vector<PGate*>& all_gates) {

    if (token.type == TokenType::Keyword && token.value == "assign") {
        PAssign* ass = new PAssign();
        ass->m_whole_exp = lexer.GetWholeExpression();
        pmod->gates[ass->m_whole_exp] = ass;
        pmod->gate_insert_order.push_back(ass->m_whole_exp);
        ass->parse(pmod);
        all_gates.push_back(ass);
    }
}

