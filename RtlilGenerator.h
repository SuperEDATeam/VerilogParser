#pragma once
#ifndef RTLIL_GENERATOR_H
#define RTLIL_GENERATOR_H

#include <string>
#include <vector>
#include <map>
#include "MyMidData.h"

// Descriptor for a single RTLIL cell instance
struct RtlilCellInfo {
    std::string instanceName; // e.g. "$and$1"
    std::string cellType;     // e.g. "$and", "$or", "$not"
    std::string inputA;       // wire connected to port A
    std::string inputB;       // wire connected to port B (empty for unary cells)
    std::string outputY;      // anonymous wire connected to port Y
    bool isUnary = false;
};

// Top-level module connect: connect <lhs> <rhs>
struct RtlilConnect {
    std::string lhs; // destination wire (with \ prefix)
    std::string rhs; // source wire
};

class RtlilGenerator {
public:
    // Generate a RTLIL file from midmap.  Returns 0 on success, -1 on error.
    int generate(MyMidModuleMap& midmap, const std::string& filePath);

private:
    int cellCounter = 0;
    std::vector<std::string>   anonWires; // anonymous intermediate wires
    std::vector<RtlilCellInfo> cells;     // collected cell descriptors
    std::vector<RtlilConnect>  connects;  // module-level connect statements

    // Recursively expand a PExpr subtree into cells.
    // Returns the wire name that carries the result of this node.
    std::string traverseExpr(PExpr* expr);

    // Map an operator symbol to its RTLIL cell type ("&" -> "$and", etc.)
    std::string opToRtlilType(const std::string& op) const;

    // Reset all internal state (allows reuse across multiple calls)
    void reset();
};

#endif
