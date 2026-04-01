#include "Optimizer.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>

// ---------- Internal helpers ----------

static std::string optTrim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static bool optSplitAssign(const std::string& expr,
                            std::string& lhs, std::string& rhs) {
    size_t pos = expr.find('=');
    if (pos == std::string::npos) return false;
    lhs = optTrim(expr.substr(0, pos));
    rhs = optTrim(expr.substr(pos + 1));
    return !lhs.empty() && !rhs.empty();
}

// Collect all wire names referenced inside a PExpr tree
static void collectUsedWires(PExpr* expr, std::unordered_set<std::string>& used) {
    if (!expr) return;
    if (auto id = dynamic_cast<PEIdent*>(expr)) {
        used.insert(id->name);
        return;
    }
    if (auto u = dynamic_cast<PEUnary*>(expr)) {
        collectUsedWires(u->expr_, used);
        return;
    }
    if (auto b = dynamic_cast<PEBinary*>(expr)) {
        collectUsedWires(b->get_left(),  used);
        collectUsedWires(b->get_right(), used);
        return;
    }
    if (auto c = dynamic_cast<PEComparison*>(expr)) {
        collectUsedWires(c->get_left(),  used);
        collectUsedWires(c->get_right(), used);
        return;
    }
}

// Build a PEUnary(op=0) wrapping a PEIdent -- represents a simple wire reference
static PExpr* makeWireRef(const std::string& wireName) {
    PEIdent* id = new PEIdent;
    id->name = wireName;
    PEUnary* u = new PEUnary;
    u->op_   = 0;
    u->expr_ = id;
    return u;
}

// ---------- optShareIR ----------

int optimizer::optShareIR(MyMidModuleMap& midmap) {
    int eliminated = 0;

    for (auto& modKv : midmap.m_module_map) {
        MyMidModule* pmod = modKv.second;
        if (!pmod) continue;

        // Maps RHS expression string -> LHS wire name of its first occurrence
        std::unordered_map<std::string, std::string> rhsToSharedWire;

        std::vector<std::string> origOrder = pmod->gate_insert_order;
        std::vector<std::string> newOrder;

        for (const auto& gname : origOrder) {
            auto it = pmod->gates.find(gname);
            if (it == pmod->gates.end()) {
                newOrder.push_back(gname);
                continue;
            }
            PGate* gate = it->second;
            if (gate->pins_.size() < 2) {
                newOrder.push_back(gname);
                continue;
            }

            std::string lhs, rhs;
            if (!optSplitAssign(gname, lhs, rhs)) {
                newOrder.push_back(gname);
                continue;
            }

            auto found = rhsToSharedWire.find(rhs);
            if (found == rhsToSharedWire.end()) {
                // First occurrence -- record it
                rhsToSharedWire[rhs] = lhs;
                newOrder.push_back(gname);
            } else {
                // Duplicate -- reuse the wire from the first occurrence
                const std::string& sharedName = found->second;

                // Replace RHS pin with a simple reference to sharedName
                gate->pins_.resize(1);
                gate->pins_.push_back(makeWireRef(sharedName));

                // Update gate key
                std::string newExp = lhs + " = " + sharedName;
                gate->m_whole_exp  = newExp;
                pmod->gates.erase(gname);
                pmod->gates[newExp] = gate;
                newOrder.push_back(newExp);

                ++eliminated;
                std::cout << "  [opt_share] '" << rhs
                          << "' -> shared wire '" << sharedName << "'\n";
            }
        }

        pmod->gate_insert_order = newOrder;
    }

    return eliminated;
}

// ---------- optConstProp ----------

int optimizer::optConstProp(MyMidModuleMap& midmap) {
    // Current AST has no literal-constant nodes (PEIdent only holds wire names).
    // Placeholder for future extension.
    (void)midmap;
    std::cout << "  [opt_const] No literal constants in design - skipping.\n";
    return 0;
}

// ---------- optClean ----------

int optimizer::optClean(MyMidModuleMap& midmap) {
    int removed = 0;

    for (auto& modKv : midmap.m_module_map) {
        MyMidModule* pmod = modKv.second;
        if (!pmod) continue;

        // Output wires are always "used"
        std::unordered_set<std::string> usedWires;
        for (auto& wkv : pmod->wires)
            if (wkv.second && wkv.second->wiretype == PWire::OUTPORT)
                usedWires.insert(wkv.first);

        // Collect all wire names referenced in any gate RHS
        for (const auto& gname : pmod->gate_insert_order) {
            auto it = pmod->gates.find(gname);
            if (it == pmod->gates.end()) continue;
            PGate* gate = it->second;
            if (gate->pins_.size() >= 2)
                collectUsedWires(gate->pins_[1], usedWires);
        }

        // Remove gates whose LHS is an unreferenced MIDDLE wire
        std::vector<std::string> toRemove;
        for (const auto& gname : pmod->gate_insert_order) {
            auto it = pmod->gates.find(gname);
            if (it == pmod->gates.end()) continue;
            PGate* gate = it->second;
            if (gate->pins_.empty()) continue;

            auto* lhsId = dynamic_cast<PEIdent*>(gate->pins_[0]);
            if (!lhsId) continue;
            const std::string& lhsName = lhsId->name;

            auto wireIt = pmod->wires.find(lhsName);
            if (wireIt != pmod->wires.end() &&
                wireIt->second->wiretype == PWire::MIDDLE &&
                usedWires.find(lhsName) == usedWires.end()) {
                toRemove.push_back(gname);
            }
        }

        for (const auto& key : toRemove) {
            pmod->gates.erase(key);
            pmod->gate_insert_order.erase(
                std::remove(pmod->gate_insert_order.begin(),
                            pmod->gate_insert_order.end(), key),
                pmod->gate_insert_order.end());

            // Remove the dead wire too
            size_t eqpos = key.find('=');
            if (eqpos != std::string::npos) {
                std::string lhs = optTrim(key.substr(0, eqpos));
                if (!lhs.empty()) pmod->wires.erase(lhs);
            }
            ++removed;
            std::cout << "  [opt_clean] removed dead gate: " << key << "\n";
        }
    }

    return removed;
}
