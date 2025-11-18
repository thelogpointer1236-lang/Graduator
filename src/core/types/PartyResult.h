#ifndef GRADUATOR_PARTYRESULT_H
#define GRADUATOR_PARTYRESULT_H

#include <vector>
#include "GaugeModel.h"
#include "core/services/graduationService/GraduationService.h"

struct PartyResult {
    GaugeModel gaugeModel;
    PressureUnit pressureUnit = PressureUnit::Kgf;
    std::vector<std::vector<grad::NodeResult>> forward;
    std::vector<std::vector<grad::NodeResult>> backward;
    std::vector<const grad::DebugData*> debugDataForward;
    std::vector<const grad::DebugData*> debugDataBackward;
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
