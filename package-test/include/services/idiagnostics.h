#pragma once
#include <string>
#include <vector>

namespace framework::services {
struct DiagnosticSnapshot {
    std::string runtimeState;
    std::vector<std::string> activeModules;
};

class IDiagnostics {
public:
    virtual ~IDiagnostics() = default;
    virtual DiagnosticSnapshot captureSnapshot() const = 0;
};
} // namespace framework::services