#pragma once
#include "Evidence.h"
#include <cstdint>
#include <string>
#include <vector>

namespace mouse_engine::core {

using EvidenceNodeId = std::uint64_t;

struct EvidenceNode {
    EvidenceNodeId id{};
    EvidenceLevel level{EvidenceLevel::Unsupported};
    std::string type;
    std::string value;
    std::string source;
};

struct EvidenceEdge {
    EvidenceNodeId from{};
    EvidenceNodeId to{};
    std::string relation;
};

struct EvidenceGraph {
    std::vector<EvidenceNode> nodes;
    std::vector<EvidenceEdge> edges;
};

} // namespace mouse_engine::core
