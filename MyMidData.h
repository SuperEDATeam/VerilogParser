#pragma once
#ifndef MYMIDDATA_H
#define MYMIDDATA_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include <list>
#include <unordered_set>
#include <memory>

using namespace std;

struct MyMidModule;

struct PWire {
    enum WireType {
        INPORT, //输入
        OUTPORT,//输出
        MIDDLE,//声明中间变量
    };
    string name;
    WireType wiretype;
    WireType get_port_type() const { return wiretype; }
    //void parser();
    //void print();
};

struct PExpr {
    virtual void parse(MyMidModule* pmod, string exp) = 0;
    //virtual void print();

};

struct PEComparison : public PExpr {
public:
    std::string get_op() const { return op_; }
    PExpr* get_left()const { return left_; }
    PExpr* get_right()const { return right_; }
    void parse(MyMidModule* pmod, string exp);


private:
    PExpr* left_;
    PExpr* right_;
    std::string op_;
    //void print();
};

struct PEBinary : public PExpr {
public:
    string get_op() const { return op_; }
    PExpr* get_left()const { return left_; }
    PExpr* get_right()const { return right_; }
    void parse(MyMidModule* rmod, string exp);
private:
    PExpr* left_;
    PExpr* right_;
    std::string op_;
    //void print();
};

struct PEUnary : public PExpr {
    char op_;
    PExpr* expr_;
    void parse(MyMidModule* pmod, string exp);
    //void print();
};

struct PEIdent : public PExpr {
    string name;
    void parse(MyMidModule* pmod, string exp);
    //void print();
};



struct PGate {
    //virtual void parser();
    //virtual void print();
    string m_whole_exp;
    vector<PExpr*> pins_;
    PExpr* pin(unsigned idx) const { return pins_[idx]; }
    //vector<PExpr*>& get_pins() { return pins_; }
};

struct PAssign : public PGate {

    void parse(MyMidModule* pmod);
    //void print();
};

struct MyMidModule {

    string modname;
    map<string, PWire*> wires;
    map<string, PGate*>  gates;
    vector<string> gate_insert_order;// 遍历gates的数组，以此为准！！！

    //const list<PGate*>& get_gates() const {
    //    return gates_;
    //}
    //string* parser();
    //void print();
    bool is_in_wires(string& name) {
        if (wires.find(name) != wires.end()) {
            return true;
        }
        return false;
    }
    const string& get_modname() const {
        return modname;
    }
};

struct MyMidModuleMap {
    map<string, MyMidModule*> m_module_map;
};




#endif