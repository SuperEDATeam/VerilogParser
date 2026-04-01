#include "Mynetlist.h"

// 字符串分割函数
std::vector<std::string> mySplit(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    size_t pos = str.find(delimiter);

    if (pos != std::string::npos) {
        result.push_back(str.substr(0, pos));
        result.push_back(str.substr(pos + 1));
    }
    else {
        result.push_back(str); // 未找到分隔符时返回原字符串
    }

    return result;
}

// 字符串修剪函数
std::string myTrim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    return str.substr(start, end - start);
}

bool MyDesign::edgeExists(const std::map<std::string, std::vector<std::string>>& edges,
    const std::string& from,
    const std::string& to) {
    auto it = edges.find(from);
    if (it != edges.end()) {
        return std::find(it->second.begin(), it->second.end(), to) != it->second.end();
    }
    return false;
}

void MyDesign::addEdge(std::map<std::string, std::vector<std::string>>& edges,
    const std::string& from,
    const std::string& to) {
    if (!edgeExists(edges,from,to)) edges[from].push_back(to);
}

int MyDesign::id2num(std::string id)
{
	if (id2num_store.count(id) > 0)
		return id2num_store[id];
	return id2num_store[id] = (int)id2num_store.size() + 1;
}

int MyDesign::getDotGateId() {
	return ++dotGateId;
}

MyScope::MyScope(void)
{

}

MyScope::~MyScope(void)
{

}

MyModule::MyModule(void)
{

}

MyModule::~MyModule(void)
{

}

MyDesign::MyDesign(void)
{

}

MyDesign::~MyDesign(void)
{

}
////////// 可视化打印AST
/* ---------- 将结点转为简短文字 ---------- */
std::string MyDesign::nodeLabel(PExpr* n)
{
    if (!n)                return "";
    if (auto id = dynamic_cast<PEIdent*>(n))         return id->name;
    if (auto u = dynamic_cast<PEUnary*>(n))         return std::string(1, u->op_);
    if (auto b = dynamic_cast<PEBinary*>(n))        return b->get_op();
    if (auto c = dynamic_cast<PEComparison*>(n))    return c->get_op();
    return "?";
}

/* ---------- 递归构造 ASCII 子树 ---------- */
MyDesign::AsciiTree MyDesign::buildTree(PExpr* node)
{
    AsciiTree at;
    if (!node) {                    // 空结点
        at.lines = { "" };
        at.width = 0;
        at.height = 0;
        at.middle = 0;
        return at;
    }

    const std::string label = nodeLabel(node);
    const int labLen = static_cast<int>(label.length());

    /* —— 收集孩子 —— */
    std::vector<PExpr*> kids;
    if (auto u = dynamic_cast<PEUnary*>(node)) {
        kids.push_back(u->expr_);
    }
    else if (auto b = dynamic_cast<PEBinary*>(node)) {
        kids.push_back(b->get_left());
        kids.push_back(b->get_right());
    }
    else if (auto c = dynamic_cast<PEComparison*>(node)) {
        kids.push_back(c->get_left());
        kids.push_back(c->get_right());
    }

    /* —— 叶子结点：直接返回 —— */
    if (kids.empty()) {
        at.lines = { label };
        at.width = labLen;
        at.height = 1;
        at.middle = labLen / 2;
        return at;
    }

    /* —— 递归获得所有子树的 ASCII 表示 —— */
    std::vector<AsciiTree> sub;
    for (auto* k : kids) sub.push_back(buildTree(k));

    /* —— 1. 计算整体宽度：孩子左右并排，中间至少 1 个空格 —— */
    int gap = 1;                               // 子树之间间隔
    int childWidth = 0;
    for (auto& s : sub) childWidth += s.width;
    childWidth += gap * (static_cast<int>(sub.size()) - 1);

    /* —— 2. 第一行：根 label 居中放在孩子区上方 —— */
    int rootPos = (sub.size() == 1)
        ? sub[0].middle               // 一棵子树，直接对齐
        : sub[0].width + gap / 2;       // 两棵子树，放在正中

    int firstWidth = std::max(rootPos + labLen, childWidth);
    std::string firstLine(firstWidth, ' ');
    firstLine.replace(rootPos, labLen, label);

    /* —— 3. 第二行：连接 / 和 \ —— */
    std::string secondLine(firstWidth, ' ');
    if (sub.size() == 1) {                  // 单孩子（Unary）：用 “/”
        int childRoot = sub[0].middle;
        int slashPos = childRoot;
        secondLine[slashPos] = '/';
    }
    else {                                // 两孩子：左 “/” 右 “\”
        int leftRoot = sub[0].middle;
        int rightRoot = sub[0].width + gap + sub[1].middle;
        secondLine[leftRoot] = '/';
        secondLine[rightRoot] = '\\';
    }

    /* —— 4. 拼接孩子的 lines —— */
    int maxChildH = 0;
    for (auto& s : sub) maxChildH = std::max(maxChildH, s.height);

    // 统一高度：短的子树用空行补齐
    for (auto& s : sub)
        while (static_cast<int>(s.lines.size()) < maxChildH)
            s.lines.push_back(std::string(s.width, ' '));

    std::vector<std::string> mergedKids(maxChildH,
        std::string(childWidth, ' '));
    // 将每棵子树贴到 mergedKids 中
    int offset = 0;
    for (size_t idx = 0; idx < sub.size(); ++idx) {
        for (int h = 0; h < maxChildH; ++h)
            mergedKids[h].replace(offset, sub[idx].width, sub[idx].lines[h]);
        offset += sub[idx].width + gap;
    }

    /* —— 5. 组装最终结果 —— */
    at.lines = { firstLine, secondLine };
    at.lines.insert(at.lines.end(), mergedKids.begin(), mergedKids.end());
    at.width = static_cast<int>(at.lines[0].size());
    at.height = static_cast<int>(at.lines.size());
    at.middle = rootPos;

    return at;
}

/* ---------- 对外接口：直接打印 ---------- */
void MyDesign::printAST(PExpr* root)
{
    std::cout << "──────────────────────────────────────────────────" << endl;
    AsciiTree tree = buildTree(root);
    for (auto& s : tree.lines)
        std::cout << s << '\n';
    std::cout << "──────────────────────────────────────────────────" << endl;
}
///////////////////////

std::string MyDesign::getOpName(const std::string& opSymbol) {
    static const std::unordered_map<std::string, std::string> op_symbol_to_name = {
        {"&", "AND"},
        {"|", "OR"},
        {"^", "XOR"},
		{"~", "NOT"},
		{"==", "EQ"},
		{"!=", "NEQ"},
		{"<", "LT"},
		{">", "GT"},
		{"<=", "LEQ"},
		{">=", "GEQ"},
        {"!", "NOT"}
    };

    auto it = op_symbol_to_name.find(opSymbol);
    if (it != op_symbol_to_name.end()) {
        return it->second;
    }
    else {
        return "Unknown";  // 如果未找到对应关系，返回默认值
    }
}

void MyDesign::print_expr_tree(PExpr* expr, int indent) {
    if (!expr) return;

    std::string space(indent, ' ');

    if (auto id = dynamic_cast<PEIdent*>(expr)) {
        std::cout << space << "Ident: " << id->name << std::endl;
    }
    else if (auto u = dynamic_cast<PEUnary*>(expr)) {
        std::cout << space << "Unary: " << u->op_ << std::endl;
        print_expr_tree(u->expr_, indent + 2);
    }
    else if (auto b = dynamic_cast<PEBinary*>(expr)) {
        std::cout << space << "Binary: " << b->get_op() << std::endl;
        print_expr_tree(b->get_left(), indent + 2);
        print_expr_tree(b->get_right(), indent + 2);
    }
    else if (auto c = dynamic_cast<PEComparison*>(expr)) {
        std::cout << space << "Compare: " << c->get_op() << std::endl;
        print_expr_tree(c->get_left(), indent + 2);
        print_expr_tree(c->get_right(), indent + 2);
    }
}

std::string MyDesign::postorder_traverse(PExpr* expr) {
    if (!expr) return "";
    if (auto id = dynamic_cast<PEIdent*>(expr)) {
        return "n" + std::to_string(id2num(id->name));
    }
    else if (auto u = dynamic_cast<PEUnary*>(expr)) {
        std::string left = postorder_traverse(u->expr_);
        return left;

    }
    else if (auto b = dynamic_cast<PEBinary*>(expr)) {
        std::string right = postorder_traverse(b->get_right());
        std::string left = postorder_traverse(b->get_left());
		std::string opr = b->get_op();
		std::string exp = left + opr + right;
        std::string opName = getOpName(opr);

        int pid1 = id2num("A");
        int pid2 = id2num("B");
        int pid3 = id2num("Y");
        int gid = id2num(exp);

        addEdge(edges, left, "c" + std::to_string(gid) + ":p" + std::to_string(pid1));
		addEdge(edges, right, "c" + std::to_string(gid) + ":p" + std::to_string(pid2));

        id2gate_store[gid] = "{{<p" + std::to_string(pid1) + ">A|" + "<p" + std::to_string(pid2) + ">B}|$" + std::to_string(getDotGateId()) + "\\" + "n$ " + getOpName(b->get_op()) + "|{<p" + std::to_string(pid3) + ">Y}}";

        return "c" + std::to_string(gid) + ":p" + std::to_string(pid3);
    }
    else if (auto c = dynamic_cast<PEComparison*>(expr)) {
        std::string right = postorder_traverse(c->get_right());
        std::string left = postorder_traverse(c->get_left());
		std::string opr = c->get_op();
		std::string exp = left + opr + right;
		std::string opName = getOpName(opr);

        int pid1 = id2num("A");
        int pid2 = id2num("B");
        int pid3 = id2num("Y");
        int gid = id2num(exp);

		addEdge(edges, left, "c" + std::to_string(gid) + ":p" + std::to_string(pid1));
		addEdge(edges, right, "c" + std::to_string(gid) + ":p" + std::to_string(pid2));

        id2gate_store[gid] = "{{<p" + std::to_string(pid1) + ">A|" + "<p" + std::to_string(pid2) + ">B}|$" + std::to_string(getDotGateId()) + "\\" + "n$ " + getOpName(c->get_op()) + "|{<p" + std::to_string(pid3) + ">Y}}";

        return "c" + std::to_string(gid) + ":p" + std::to_string(pid3);
    }
}

int MyDesign::My_show_netlist(MyMidModuleMap midmap_,const std::string& filePath)
{
    /* ── 1. 选模块 ─────────────────────── */
    std::string modName;
    std::map<std::string, PWire*> wiresMap;
    std::map<std::string, PGate*> gatesMap;
    std::vector<std::string> gateOrder;

    for (auto& kv : midmap_.m_module_map) {
        if (kv.second) {
            modName = kv.second->modname;
            wiresMap = kv.second->wires;
            gatesMap = kv.second->gates;
            gateOrder = kv.second->gate_insert_order;
            break;
        }
    }

    /* ── 2. 打开文件 ───────────────────── */
    FILE* dotfile = nullptr;
    if (fopen_s(&dotfile, filePath.c_str(), "w") || !dotfile) {
        std::fprintf(stderr, "failed to open show.dot\n");
        return -1;
    }

    /* ── 3. 头部 ───────────────────────── */
    std::fprintf(dotfile,
        "digraph \"%s\" {\n"
        "label=\"%s\";\n"
        "rankdir=\"LR\";\n"
        "remincross=true;\n",
        modName.c_str(), modName.c_str());

    /* ── 4. wires：IN/OUT→octagon，MIDDLE→diamond ─ */
    for (auto& wkv : wiresMap) {
        const auto* w = wkv.second;
        int  nid = id2num(wkv.first);
        const char* sh = (w->wiretype == PWire::MIDDLE ? "diamond" : "octagon");
        std::fprintf(dotfile,
            "n%d [ shape=%s, label=\"%s\" ];\n",
            nid, sh, wkv.first.c_str());
    }


    /* ── 5. gates ───────────────────────── */
    for (const auto& gname : gateOrder) {
        PGate* gate = gatesMap[gname];
        //int    gid = id2num(gname);

        /* 5-1 统计端口数 (= 连续 PEIdent) */
        size_t port_cnt = 0;
        while (port_cnt < gate->pins_.size() &&
            dynamic_cast<PEIdent*>(gate->pins_[port_cnt]))
            ++port_cnt;

        size_t in_cnt = port_cnt / 2;
        size_t out_cnt = port_cnt - in_cnt;

        /* 5-2 表达式树 */
        PExpr* expr_root = (port_cnt < gate->pins_.size())
            ? gate->pins_[port_cnt]
            : nullptr;

        /* 5-3 组合 record */

        //std::cout << "$$$$$$$$$$" << endl;
        //print_expr_tree(expr_root);
        //std::cout << "$$$$$$$$$$" << endl;

		printAST(expr_root);

        addEdge(edges, postorder_traverse(expr_root), postorder_traverse(gate->pins_[0]));
    }
    // 遍历添加所有门节点信息
    int opId = 0;
    for (const auto& pair : id2gate_store) {
        std::fprintf(dotfile,
            "c%d [ shape=record, label=\"%s\", ];\n",
            pair.first, pair.second.c_str());
    }

    /* ── 6. edges ───────────────────────── */

        for (const auto& pair : edges) {
            const std::string& from = pair.first;
            const std::vector<std::string>& to_list = pair.second;
            for (const auto& to : to_list) {
                std::fprintf(dotfile, "%s:e -> %s:w;\n", from.c_str(), to.c_str());
            }
        }

    std::fprintf(dotfile, "}\n");
    std::fclose(dotfile);
    std::cout << "Dot file generated successfully." << endl;
    return 0;
}

//递归获取表达式
std::string MyDesign::getNameRecursively(PExpr* expr) {
    if (!expr) return "";
    if (auto id = dynamic_cast<PEIdent*>(expr)) {
        return id->name;
    }
    else if (auto u = dynamic_cast<PEUnary*>(expr)) {
        std::string left = getNameRecursively(u->expr_);
        return left;
    }
    else if (auto b = dynamic_cast<PEBinary*>(expr)) {
        std::string right = getNameRecursively(b->get_right());
        std::string left = getNameRecursively(b->get_left());
        std::string opr = b->get_op();
        std::string exp = left + " " + opr + " " + right;
        //std::cout << "exp:" + exp + "~" << endl;
        return exp;
    }
    else if (auto c = dynamic_cast<PEComparison*>(expr)) {
        std::string right = getNameRecursively(c->get_right());
        std::string left = getNameRecursively(c->get_left());
        std::string opr = c->get_op();
        std::string exp = left + " " + opr + " " + right;
        //std::cout << "exp:" + exp + "~" << endl;
        return exp;
    }
}

std::string MyDesign::postorder_blif_traverse(PExpr* expr, FILE* blifFile) {
    if (!expr) return "";
    if (auto id = dynamic_cast<PEIdent*>(expr)) {
        return getNameRecursively(expr);
    }
    else if (auto u = dynamic_cast<PEUnary*>(expr)) {
		std::string left = getNameRecursively(expr);
        std::fprintf(blifFile, ".names %s %s\n", left.c_str(), blifNameAllocator.allocateName(left.c_str()).c_str());
        std::fprintf(blifFile, "1 1\n");
        return getNameRecursively(expr);
    }
    else if (auto b = dynamic_cast<PEBinary*>(expr)) {
        std::string right = postorder_blif_traverse(b->get_right(), blifFile);
        std::string left = postorder_blif_traverse(b->get_left(), blifFile);
        std::string opr = b->get_op();
        std::string opName = getOpName(opr);

        std::string name = blifNameAllocator.allocateName(getNameRecursively(expr));

        if (opName == "AND") {
			std::fprintf(blifFile, ".names %s %s %s\n", blifNameAllocator.allocateName(left.c_str()).c_str(), blifNameAllocator.allocateName(right.c_str()).c_str(), name.c_str());
			std::fprintf(blifFile, "11 1\n");
		}
        else if (opName == "OR") {
            std::fprintf(blifFile, ".names %s %s %s\n", blifNameAllocator.allocateName(left.c_str()).c_str(), blifNameAllocator.allocateName(right.c_str()).c_str(), name.c_str());
            std::fprintf(blifFile, "1- 1\n");
			std::fprintf(blifFile, "-1 1\n");
        }
        else if (opName == "NOT") {
            std::fprintf(blifFile, ".names %s %s\n", blifNameAllocator.allocateName(left.c_str()).c_str(), name.c_str());
            std::fprintf(blifFile, "0 1\n");
        }
        return  getNameRecursively(expr);
    }
    else if (auto c = dynamic_cast<PEComparison*>(expr)) {
        std::string right = postorder_blif_traverse(c->get_right(), blifFile);
        std::string left = postorder_blif_traverse(c->get_left(), blifFile);
        std::string opr = c->get_op();
        std::string opName = getOpName(opr);

        std::string name = blifNameAllocator.allocateName(getNameRecursively(expr));

        if (opName == "AND") {
            std::fprintf(blifFile, ".names %s %s %s\n", blifNameAllocator.allocateName(left.c_str()).c_str(), blifNameAllocator.allocateName(right.c_str()).c_str(), name.c_str());
            std::fprintf(blifFile, "11 1\n");
        }
        else if (opName == "OR") {
            std::fprintf(blifFile, ".names %s %s %s\n", blifNameAllocator.allocateName(left.c_str()).c_str(), blifNameAllocator.allocateName(right.c_str()).c_str(), name.c_str());
            std::fprintf(blifFile, "1- 1\n");
            std::fprintf(blifFile, "-1 1\n");
        }
        else if (opName == "NOT") {
            std::fprintf(blifFile, ".names %s %s\n", blifNameAllocator.allocateName(left.c_str()).c_str(), name.c_str());
            std::fprintf(blifFile, "0 1\n");
        }
        return  getNameRecursively(expr);
    }
}

int MyDesign::My_show_blif(MyMidModuleMap midmap_, const std::string& filePath) {

    /* ── 1. 选模块 ─────────────────────── */
    std::string modName;
    std::map<std::string, PWire*> wiresMap;
    std::map<std::string, PGate*> gatesMap;
    std::vector<std::string> gateOrder;
    for (auto& kv : midmap_.m_module_map) {
        if (kv.second) {
            modName = kv.second->modname;
            wiresMap = kv.second->wires;
            gatesMap = kv.second->gates;
            gateOrder = kv.second->gate_insert_order;
            break;
        }
    }

    /* ── 2. 打开文件 ───────────────────── */
    FILE* blifFile = nullptr;
    if (fopen_s(&blifFile, filePath.c_str(), "w") || !blifFile) {
        std::fprintf(stderr, "failed to open \".blif\"\n");
        return -1;
    }

    /* ── 3. model ───────────────────────── */
    std::fprintf(blifFile,
        ".model %s\n",
        modName.c_str());

    /* ── 4. inputs outputs ─ */
    std::string inputs = ".inputs";
	std::string outputs = ".outputs";
    for (auto& wkv : wiresMap) {
        const auto* w = wkv.second;
        if (w->wiretype == PWire::INPORT) {
            blifNameAllocator.registerName(wkv.first, wkv.first); // 注册 wire 名称
			inputs += " " + wkv.first;
        }
        else if (w->wiretype == PWire::MIDDLE) {
            blifNameAllocator.registerName(wkv.first, wkv.first); // 注册 wire 名称
        }
        else if (w->wiretype == PWire::OUTPORT) {
			outputs += " " + wkv.first;
        }
    }
    std::fprintf(blifFile,
        "%s\n"
        "%s\n",
        inputs.c_str(), outputs.c_str());

    /* ── 5. gates ───────────────────────── */
    for (const auto& gname : gateOrder) {
        PGate* gate = gatesMap[gname];
        auto parts = mySplit(gate->m_whole_exp, '=');
        if (parts.size() == 2) {
            std::string lhs = myTrim(parts[0]);
            std::string rhs = myTrim(parts[1]);
            blifNameAllocator.registerName(rhs, lhs);   // 注册表达式名称
        }
    }

    for (const auto& gname : gateOrder) {
        PGate* gate = gatesMap[gname];

        /* 5-1 统计端口数 (= 连续 PEIdent) */
        size_t port_cnt = 0;
        while (port_cnt < gate->pins_.size() &&
            dynamic_cast<PEIdent*>(gate->pins_[port_cnt]))
            ++port_cnt;

        size_t in_cnt = port_cnt / 2;
        size_t out_cnt = port_cnt - in_cnt;

        /* 5-2 表达式树 */
        PExpr* expr_root = (port_cnt < gate->pins_.size())
            ? gate->pins_[port_cnt]
            : nullptr;

        //std::cout<< gname <<endl;

        /* 5-3 组合 record */
        addEdge(edges, postorder_blif_traverse(expr_root,blifFile), postorder_traverse(gate->pins_[0]));
    }

    std::fprintf(blifFile, ".end");
    std::fclose(blifFile);
    std::cout << "Blif file generated successfully." << endl;
    return 0;
}





