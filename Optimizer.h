#pragma once
#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "MyMidData.h"

namespace optimizer {

// Shared sub-expression elimination (IR level).
// Scans all assign gates; when the same RHS expression appears more than once,
// subsequent occurrences are replaced with a reference to the wire driven by
// the first occurrence.  The AST is modified in-place; no re-parsing needed.
// Returns the number of redundant expressions removed.
int optShareIR(MyMidModuleMap& midmap);

// Constant propagation.
// Placeholder -- current AST has no literal-constant nodes.
// Returns the number of substitutions made.
int optConstProp(MyMidModuleMap& midmap);

// Dead-code elimination.
// Removes MIDDLE wires (and their driving gates) that are never read by any
// other gate or output port.
// Returns the number of gates removed.
int optClean(MyMidModuleMap& midmap);

} // namespace optimizer

#endif
