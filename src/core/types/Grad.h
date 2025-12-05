#ifndef GRADUATOR_GRAD_H
#define GRADUATOR_GRAD_H

struct DebugData {
    // локальные данные для каждого узла
    struct LocalWindow {
        double nodePressure;
        std::vector<double> localPressure;
        std::vector<double> localAngle;
    };
    std::vector<LocalWindow> windows;
};

struct NodeResult {
    double pressure;
    double angle;
    bool   valid;
};

#endif //GRADUATOR_GRAD_H