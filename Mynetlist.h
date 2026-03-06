#pragma once
#ifndef MYNETLIST_H
#define MYNETLIST_H

#include <map>
#include <set>
#include <unordered_map>
#include <string>
#include <vector>
#include <cctype>
#include <memory>
#include <iostream>

#include "utils.h"

#include "MyMidData.h"
#include "BlifNameAllocator.h"

using namespace std;

class MyDesign;
class MyScope;
class MyModule;


using IdString = std::string;
//typedef std::string IdString;

enum MY_PORT_TYPE { NOT_A_PORT2, PIMPLICIT2, PINPUT2, POUTPUT2, PINOUT2, PREF2 };


struct MYTMPNODE
{
    IdString name;
};

struct MYWIRE
{
    IdString name;
    MY_PORT_TYPE porttype;
};

struct MYPORT
{
    IdString     portname;
    char        inout_type;  //0:in, 1:out
};

struct MYCELL
{
    IdString name;
    IdString type;
    std::map<IdString, MYPORT> cell_conns;  //<wirename,port>
};


class MyScope {

public:
    MyScope();
    ~MyScope();
    std::vector<MYWIRE> m_wires;
    std::vector<MYPORT> m_ports;
    std::vector<MYCELL> m_cells;
    std::vector<MYTMPNODE> m_tmpnodes;

    //void set_num_ports(unsigned int num_ports);
private:

};

class MyModule : public MyScope {

public:
    MyModule();
    ~MyModule();
};

class MyDesign {

public:
    MyDesign();
    ~MyDesign();

    MyModule m_mod;

	BlifNameAllocator blifNameAllocator;  // 用于分配BLIF门的名称

    // 为每个id分配一个唯一的数字ID
	int id2num(std::string id);

    int getDotGateId();

	// 对于操作符字符串，返回其对应的名称
    std::string getOpName(const std::string& opSymbol);

	// 判定dot文件中的边是否存在
    bool edgeExists(const std::map<std::string, std::vector<std::string>>& edges,
        const std::string& from,
        const std::string& to);

	// 添加边到dot文件的边集
    void addEdge(std::map<std::string, std::vector<std::string>>& edges,
        const std::string& from,
        const std::string& to);
    
	// 后序遍历(先右子节点！！！)生成对应的 record-label 以及完善 dot 边集
    std::string postorder_traverse(PExpr* expr);

	// 打印表达式树
    void print_expr_tree(PExpr* expr, int indent = 0);

	// 生成 dot 文件
    int My_show_netlist(MyMidModuleMap midmap_, const std::string& fileName);

    // 后序遍历(先右子节点！！！)生成对应的 blif
    std::string postorder_blif_traverse(PExpr* expr, FILE*);

    //递归获取字符串名（）用于生成blif文件
    std::string getNameRecursively(PExpr* expr);

    // 生成 blif 文件
	int My_show_blif(MyMidModuleMap midmap_, const std::string& fileName);

    /// 打印整个表达式 AST
    static void printAST(PExpr* root);

private:

    struct AsciiTree {                    // 递归中间结果
        std::vector<std::string> lines;   // 每一行的字符串
        int width = 0;                   // 总宽度
        int height = 0;                   // 总高度
        int middle = 0;                   // 根结点在 lines[0] 中的列号
    };

    static std::string nodeLabel(PExpr* n);          // 结点文字
    static AsciiTree   buildTree(PExpr* n);          // 递归构造 ASCII 图

    std::map<std::string, std::vector<std::string>> edges;
    std::map<int, std::string> id2gate_store;
    std::map<std::string, int> id2num_store;

    int dotGateId = 0;
};

#endif
