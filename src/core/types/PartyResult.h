#ifndef GRADUATOR_PARTYRESULT_H
#define GRADUATOR_PARTYRESULT_H

#include <vector>

struct PartyResult {
    std::vector<std::vector<double>> forward;
    std::vector<std::vector<double>> backward;

    [[nodiscard]] bool isValid() const {
        return !forward.empty() || !backward.empty();
    }

    void clear() {
        forward.clear();
        backward.clear();
    }
};

#endif // GRADUATOR_PARTYRESULT_H
