#ifndef GRADUATOR_PARTYRESULT_H
#define GRADUATOR_PARTYRESULT_H

#include <vector>
#include "GaugeModel.h"
#include "Grad.h"

struct PartyResult {
    GaugeModel gaugeModel;
    std::vector<std::vector<NodeResult>> forward;
    std::vector<std::vector<NodeResult>> backward;
    std::vector<const DebugData*> debugDataForward;
    std::vector<const DebugData*> debugDataBackward;
    double durationSeconds;

    [[nodiscard]] bool isValid() const {
        return !forward.empty() || !backward.empty();
    }

    void clear() {
        forward.clear();
        backward.clear();
    }
};

#endif // GRADUATOR_PARTYRESULT_H
