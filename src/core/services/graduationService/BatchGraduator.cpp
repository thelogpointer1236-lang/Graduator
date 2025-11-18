#include "BatchGraduator.h"
#include <stdexcept>

using namespace grad;

BatchGraduator::BatchGraduator(int cameraCount) {
    if (cameraCount <= 0) {
        throw std::runtime_error("cameraCount must be positive.");
    }

    for (int i = 0; i < cameraCount; ++i) {
        m_calibrators.push_back(PressureAngleGraduator(m_pressureSeries));
    }
}

BatchGraduator::~BatchGraduator() = default;

void BatchGraduator::addPressureSample(double time, double pressure) {
    m_pressureSeries.emplace_back(time, pressure);
}

void BatchGraduator::addAngleSample(int cameraIndex, double time, double angle) {
    if (cameraIndex < 0 || cameraIndex >= static_cast<int>(m_calibrators.size())) {
        throw std::runtime_error("Invalid cameraIndex.");
    }
    m_calibrators[cameraIndex].addAngleSample(time, angle);
}

void BatchGraduator::setNodePressures(const std::vector<double>& pressures) {
    for (auto& calib : m_calibrators) {
        calib.setNodePressures(pressures);
    }
}

void BatchGraduator::setPressureWindow(double window) {
    for (auto& calib : m_calibrators) {
        calib.setPressureWindow(window);
    }
}

void BatchGraduator::setMinPoints(std::size_t minPoints) {
    for (auto& calib : m_calibrators) {
        calib.setMinPoints(minPoints);
    }
}

void BatchGraduator::setLoessFrac(double frac) {
    for (auto& calib : m_calibrators) {
        calib.setLoessFrac(frac);
    }
}

std::vector<std::vector<grad::NodeResult>> BatchGraduator::graduate() const {
    std::vector<std::vector<grad::NodeResult>> out;
    out.reserve(m_calibrators.size());

    for (auto const& calib : m_calibrators) {
        out.push_back(calib.graduate());
    }

    return out;
}

std::vector<double> BatchGraduator::scaleAngleRange() const {
    std::vector<double> out;
    out.reserve(m_calibrators.size());
    for (auto const& c : m_calibrators) {
        out.push_back(c.scaleAngleRange());
    }
    return out;
}

std::vector<double> BatchGraduator::scaleNonlinearity() const {
    std::vector<double> out;
    out.reserve(m_calibrators.size());
    for (auto const& c : m_calibrators) {
        out.push_back(c.scaleNonlinearity());
    }
    return out;
}

std::vector<std::size_t> BatchGraduator::anglesCount() const {
    std::vector<std::size_t> out;
    out.reserve(m_calibrators.size());
    for (auto const& c : m_calibrators) {
        out.push_back(c.anglesCount());
    }
    return out;
}

std::vector<const grad::DebugData *> BatchGraduator::allDebugData() const {
    std::vector<const grad::DebugData*> out;
    out.reserve(m_calibrators.size());

    for (auto const& c : m_calibrators) {
        out.push_back(&c.debugData());
    }

    return out;
}

void BatchGraduator::clear() {
    m_pressureSeries.clear();
    for (auto& calib : m_calibrators) {
        calib.clear();
    }
}
