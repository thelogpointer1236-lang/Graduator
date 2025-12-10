#ifndef GRADUATOR_PARTYRESULT_H
#define GRADUATOR_PARTYRESULT_H

#include <vector>
#include <limits>

#include <QString>
#include <QStringList>

#include "GaugeModel.h"
#include "Grad.h"

struct PartyValidationIssue {
    enum class Severity {
        Info,
        Warning,
        Error
    };

    enum class Category {
        InvalidValue,
        AngleRange,
        Hysteresis
    };

    int cameraIndex = -1;
    bool forward = true;
    int row = -1;
    Severity severity = Severity::Info;
    Category category = Category::InvalidValue;
    QString message;
};

struct PartyValidationResult {
    std::vector<PartyValidationIssue> issues;
    QStringList messages;

    [[nodiscard]] bool ok() const {
        for (const auto &issue : issues) {
            if (issue.severity == PartyValidationIssue::Severity::Error) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] const PartyValidationIssue *issueFor(int camIdx, bool isForward, int row) const {
        for (const auto &issue : issues) {
            if (issue.cameraIndex == camIdx && issue.forward == isForward && issue.row == row) {
                return &issue;
            }
        }
        return nullptr;
    }
};

struct PartyResult {
    GaugeModel gaugeModel;

    std::vector<std::vector<NodeResult>> forward;
    std::vector<std::vector<NodeResult>> backward;

    std::vector<double> nolinForward;
    std::vector<double> nolinBackward;

    std::vector<const DebugData*> debugDataForward;
    std::vector<const DebugData*> debugDataBackward;
    double durationSeconds = 0.0;

    bool strongNode = false;
    double precisionClass = std::numeric_limits<double>::quiet_NaN();

    [[nodiscard]] PartyValidationResult validate() const;

    void clear() {
        forward.clear();
        backward.clear();
        nolinForward.clear();
        nolinBackward.clear();
        debugDataForward.clear();
        debugDataBackward.clear();
        durationSeconds = 0.0;
        strongNode = false;
        precisionClass = std::numeric_limits<double>::quiet_NaN();
    }
};

#endif // GRADUATOR_PARTYRESULT_H
