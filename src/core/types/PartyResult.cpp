#include "PartyResult.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QtMath>

namespace {
    constexpr double kMinAngleDeg = 260.0;
    constexpr double kMaxAngleDeg = 280.0;

    double toDegrees(double radians) {
        return qRadiansToDegrees(radians);
    }

    bool isFormed(const std::vector<NodeResult> &nodes, std::size_t expectedCount) {
        return nodes.size() == expectedCount;
    }
}

PartyValidationResult PartyResult::validate() const {
    PartyValidationResult validation;

    const std::size_t nodeCount = gaugeModel.pressureValues().size();
    if (nodeCount == 0) {
        return validation;
    }

    auto checkDirection = [&](const std::vector<std::vector<NodeResult>> &direction, bool isForward) {
        for (std::size_t cam = 0; cam < direction.size(); ++cam) {
            const auto &nodes = direction[cam];
            if (!isFormed(nodes, nodeCount)) {
                continue;
            }

            // Проверяем, что в сформированной колонке нет nan/0
            for (std::size_t node = 0; node < nodeCount; ++node) {
                const double angle = nodes[node].angle;
                if (!std::isfinite(angle) || qFuzzyIsNull(angle)) {
                    PartyValidationIssue issue;
                    issue.cameraIndex = static_cast<int>(cam);
                    issue.forward = isForward;
                    issue.row = static_cast<int>(node);
                    issue.severity = PartyValidationIssue::Severity::Error;
                    issue.category = PartyValidationIssue::Category::InvalidValue;
                    issue.message = QString::fromWCharArray(L"Недопустимое значение угла (nan/0). Манометр не будет сохранён.");
                    validation.issues.push_back(issue);
                }
            }

            // Диапазон угла
            if (!strongNode && nodes.size() >= 2) {
                const double angleDeg = toDegrees(nodes.back().angle - nodes.front().angle);
                if (angleDeg < kMinAngleDeg || angleDeg > kMaxAngleDeg) {
                    PartyValidationIssue issue;
                    issue.cameraIndex = static_cast<int>(cam);
                    issue.forward = isForward;
                    issue.row = static_cast<int>(nodeCount); // строка "Common"
                    issue.severity = PartyValidationIssue::Severity::Error;
                    issue.category = PartyValidationIssue::Category::AngleRange;
                    issue.message = QString::fromWCharArray(L"Общий угол вне диапазона 260–280°.");
                    validation.issues.push_back(issue);
                }
            }
        }
    };

    checkDirection(forward, true);
    checkDirection(backward, false);

    // Гистерезис
    if (std::isfinite(precisionClass)) {
        const std::size_t cameras = std::min(forward.size(), backward.size());
        for (std::size_t cam = 0; cam < cameras; ++cam) {
            const auto &fw = forward[cam];
            const auto &bw = backward[cam];
            if (!isFormed(fw, nodeCount) || !isFormed(bw, nodeCount)) {
                continue;
            }

            const double forwardRangeDeg = (fw.size() >= 2)
                                               ? toDegrees(fw.back().angle - fw.front().angle)
                                               : std::numeric_limits<double>::quiet_NaN();
            const double backwardRangeDeg = (bw.size() >= 2)
                                                ? toDegrees(bw.back().angle - bw.front().angle)
                                                : std::numeric_limits<double>::quiet_NaN();

            double baseRangeDeg = std::isfinite(forwardRangeDeg) ? forwardRangeDeg : backwardRangeDeg;
            if (!std::isfinite(baseRangeDeg) || qFuzzyIsNull(baseRangeDeg)) {
                continue;
            }

            const double allowedDiffDeg = baseRangeDeg * (precisionClass / 100.0);
            for (std::size_t node = 0; node < nodeCount; ++node) {
                const double angleFw = fw[node].angle;
                const double angleBw = bw[node].angle;
                if (!std::isfinite(angleFw) || !std::isfinite(angleBw)) {
                    continue;
                }

                const double hysteresisDeg = std::abs(toDegrees(angleFw - angleBw));
                if (hysteresisDeg > allowedDiffDeg) {
                    PartyValidationIssue issue;
                    issue.cameraIndex = static_cast<int>(cam);
                    issue.forward = true;
                    issue.row = static_cast<int>(node);
                    issue.severity = PartyValidationIssue::Severity::Warning;
                    issue.category = PartyValidationIssue::Category::Hysteresis;
                    issue.message = QString::fromWCharArray(L"Гистерезис превышает выбранный класс точности.");
                    validation.issues.push_back(issue);
                }
            }
        }
    }

    for (const auto &issue : validation.issues) {
        validation.messages << issue.message;
    }

    return validation;
}
