#include "RtlilGenerator.h"
#include <cstdio>
#include <algorithm>

// ---------- Internal helpers ----------

static std::string rtlTrim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Split "lhs = rhs" into separate strings
static bool splitAssign(const std::string& expr,
                        std::string& lhs, std::string& rhs) {
    size_t pos = expr.find('=');
    if (pos == std::string::npos) return false;
    lhs = rtlTrim(expr.substr(0, pos));
    rhs = rtlTrim(expr.substr(pos + 1));
    return !lhs.empty();
}

// ---------- RtlilGenerator ----------

void RtlilGenerator::reset() {
    cellCounter = 0;
    anonWires.clear();
    cells.clear();
    connects.clear();
}

std::string RtlilGenerator::opToRtlilType(const std::string& op) const {
    static const std::map<std::string, std::string> table = {
        {"&",  "$and"},
        {"|",  "$or"},
        {"^",  "$xor"},
        {"~",  "$not"},
        {"!",  "$not"},
        {"==", "$eq"},
        {"!=", "$ne"},
        {"<",  "$lt"},
        {">",  "$gt"},
        {"<=", "$le"},
        {">=", "$ge"},
    };
    auto it = table.find(op);
    return (it != table.end()) ? it->second : "$unknown";
}

// Post-order traversal: emit cells for every sub-expression,
// return the wire name that carries the result of this node.
std::string RtlilGenerator::traverseExpr(PExpr* expr) {
    if (!expr) return "";

    // Leaf: named signal wire
    if (auto id = dynamic_cast<PEIdent*>(expr)) {
        return "\\" + id->name;
    }

    // Unary: op_==0 means simple pass-through; otherwise NOT
    if (auto u = dynamic_cast<PEUnary*>(expr)) {
        std::string childWire = traverseExpr(u->expr_);
        if (u->op_ == 0) {
            return childWire; // no cell needed for plain assignment
        }
        ++cellCounter;
        std::string outWire = "$" + std::to_string(cellCounter) + "_Y";
        anonWires.push_back(outWire);

        RtlilCellInfo ci;
        ci.instanceName = opToRtlilType(std::string(1, u->op_)) + "$" + std::to_string(cellCounter);
        ci.cellType     = opToRtlilType(std::string(1, u->op_));
        ci.inputA       = childWire;
        ci.outputY      = outWire;
        ci.isUnary      = true;
        cells.push_back(ci);
        return outWire;
    }

    // Binary operator
    if (auto b = dynamic_cast<PEBinary*>(expr)) {
        std::string leftWire  = traverseExpr(b->get_left());
        std::string rightWire = traverseExpr(b->get_right());

        ++cellCounter;
        std::string outWire = "$" + std::to_string(cellCounter) + "_Y";
        anonWires.push_back(outWire);

        RtlilCellInfo ci;
        ci.instanceName = opToRtlilType(b->get_op()) + "$" + std::to_string(cellCounter);
        ci.cellType     = opToRtlilType(b->get_op());
        ci.inputA       = leftWire;
        ci.inputB       = rightWire;
        ci.outputY      = outWire;
        ci.isUnary      = false;
        cells.push_back(ci);
        return outWire;
    }

    // Comparison operator (structurally identical to binary)
    if (auto c = dynamic_cast<PEComparison*>(expr)) {
        std::string leftWire  = traverseExpr(c->get_left());
        std::string rightWire = traverseExpr(c->get_right());

        ++cellCounter;
        std::string outWire = "$" + std::to_string(cellCounter) + "_Y";
        anonWires.push_back(outWire);

        RtlilCellInfo ci;
        ci.instanceName = opToRtlilType(c->get_op()) + "$" + std::to_string(cellCounter);
        ci.cellType     = opToRtlilType(c->get_op());
        ci.inputA       = leftWire;
        ci.inputB       = rightWire;
        ci.outputY      = outWire;
        ci.isUnary      = false;
        cells.push_back(ci);
        return outWire;
    }

    return "";
}

int RtlilGenerator::generate(MyMidModuleMap& midmap, const std::string& filePath) {
    reset();

    // -- 1. Pick the first (only) module
    std::string modName;
    std::map<std::string, PWire*> wiresMap;
    std::map<std::string, PGate*> gatesMap;
    std::vector<std::string>      gateOrder;

    for (auto& kv : midmap.m_module_map) {
        if (kv.second) {
            modName   = kv.second->modname;
            wiresMap  = kv.second->wires;
            gatesMap  = kv.second->gates;
            gateOrder = kv.second->gate_insert_order;
            break;
        }
    }

    // -- 2. Classify wires
    std::vector<PWire*> inputWires, outputWires, middleWires;
    for (auto& wkv : wiresMap) {
        switch (wkv.second->wiretype) {
            case PWire::INPORT:  inputWires.push_back(wkv.second);  break;
            case PWire::OUTPORT: outputWires.push_back(wkv.second); break;
            case PWire::MIDDLE:  middleWires.push_back(wkv.second); break;
        }
    }
    auto byName = [](const PWire* a, const PWire* b) { return a->name < b->name; };
    std::sort(inputWires.begin(),  inputWires.end(),  byName);
    std::sort(outputWires.begin(), outputWires.end(), byName);
    std::sort(middleWires.begin(), middleWires.end(), byName);

    // -- 3. Traverse all gates to collect cells and connects
    for (const auto& gname : gateOrder) {
        auto it = gatesMap.find(gname);
        if (it == gatesMap.end()) continue;
        PGate* gate = it->second;
        if (gate->pins_.size() < 2) continue;

        auto* lhsIdent = dynamic_cast<PEIdent*>(gate->pins_[0]);
        if (!lhsIdent) continue;
        std::string lhsName = lhsIdent->name;

        std::string resultWire = traverseExpr(gate->pins_[1]);

        RtlilConnect conn;
        conn.lhs = "\\" + lhsName;
        conn.rhs = resultWire;
        connects.push_back(conn);
    }

    // -- 4. Open output file
    FILE* f = nullptr;
    if (fopen_s(&f, filePath.c_str(), "w") || !f) {
        std::fprintf(stderr, "RtlilGenerator: cannot open '%s'\n", filePath.c_str());
        return -1;
    }

    // -- 5. Header
    std::fprintf(f, "autoidx %d\n", cellCounter + 1);
    std::fprintf(f, "attribute \\cells_not_processed 1\n");
    std::fprintf(f, "module \\%s\n", modName.c_str());

    // -- 6. Anonymous wires (cell outputs)
    for (const auto& w : anonWires)
        std::fprintf(f, "  wire %s\n", w.c_str());

    // -- 7. Named wires: inputs -> outputs -> middle
    int portIdx = 1;
    for (const auto* w : inputWires)
        std::fprintf(f, "  wire input %d \\%s\n", portIdx++, w->name.c_str());
    for (const auto* w : outputWires)
        std::fprintf(f, "  wire output %d \\%s\n", portIdx++, w->name.c_str());
    for (const auto* w : middleWires)
        std::fprintf(f, "  wire \\%s\n", w->name.c_str());

    // -- 8. Cells
    for (const auto& ci : cells) {
        std::fprintf(f, "  cell %s %s\n", ci.cellType.c_str(), ci.instanceName.c_str());
        if (ci.isUnary) {
            std::fprintf(f, "    parameter \\A_SIGNED 0\n");
            std::fprintf(f, "    parameter \\A_WIDTH 1\n");
            std::fprintf(f, "    parameter \\Y_WIDTH 1\n");
            std::fprintf(f, "    connect \\A %s\n", ci.inputA.c_str());
        } else {
            std::fprintf(f, "    parameter \\A_SIGNED 0\n");
            std::fprintf(f, "    parameter \\A_WIDTH 1\n");
            std::fprintf(f, "    parameter \\B_SIGNED 0\n");
            std::fprintf(f, "    parameter \\B_WIDTH 1\n");
            std::fprintf(f, "    parameter \\Y_WIDTH 1\n");
            std::fprintf(f, "    connect \\A %s\n", ci.inputA.c_str());
            std::fprintf(f, "    connect \\B %s\n", ci.inputB.c_str());
        }
        std::fprintf(f, "    connect \\Y %s\n", ci.outputY.c_str());
        std::fprintf(f, "  end\n");
    }

    // -- 9. Top-level connect statements
    for (const auto& conn : connects) {
        if (conn.lhs == conn.rhs) continue; // skip trivial self-connects
        std::fprintf(f, "  connect %s %s\n", conn.lhs.c_str(), conn.rhs.c_str());
    }

    std::fprintf(f, "end\n");
    std::fclose(f);

    std::printf("RTLIL generated: %s\n", filePath.c_str());
    return 0;
}
