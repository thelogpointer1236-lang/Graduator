#include "Graduator.h"

using namespace grad;

Graduator::Graduator(int cameraCount)
    : m_forward(cameraCount)
    , m_cameraCount(cameraCount)
    , m_backward(cameraCount) {
}

void Graduator::switchToForward() {
    m_isForward = true;
}

void Graduator::switchToBackward() {
    m_isForward = false;
}

bool Graduator::isForward() const {
    return m_isForward;
}

BatchGraduator& Graduator::active() {
    return m_isForward ? m_forward : m_backward;
}

const BatchGraduator& Graduator::active() const {
    return m_isForward ? m_forward : m_backward;
}

// ------------------- Proxy API -------------------

void Graduator::addPressureSample(double time, double pressure) {
    m_forward.addPressureSample(time, pressure);
    m_backward.addPressureSample(time, pressure);
}

void Graduator::addAngleSample(int cameraIndex, double time, double angle) {
    if (cameraIndex < 0 || static_cast<std::size_t>(cameraIndex) >= m_cameraCount) {
        return;
    }
    active().addAngleSample(cameraIndex, time, angle);
}

void Graduator::setNodePressures(const std::vector<double>& pressures) {
    m_forward.setNodePressures(pressures);
    m_backward.setNodePressures(pressures);
}

void Graduator::setPressureWindow(double window) {
    m_forward.setPressureWindow(window);
    m_backward.setPressureWindow(window);
}

void Graduator::setMinPoints(std::size_t minPoints) {
    m_forward.setMinPoints(minPoints);
    m_backward.setMinPoints(minPoints);
}

void Graduator::setLoessFrac(double frac) {
    m_forward.setLoessFrac(frac);
    m_backward.setLoessFrac(frac);
}

std::vector<std::vector<NodeResult>> Graduator::graduateForward() const {
    return m_forward.graduate();
}

std::vector<std::vector<NodeResult>> Graduator::graduateBackward() const {
    return m_backward.graduate();
}

std::vector<double> Graduator::scaleAngleRangeForward() const {
    return m_forward.scaleAngleRange();
}

std::vector<double> Graduator::scaleAngleRangeBackward() const {
    return m_backward.scaleAngleRange();
}

std::vector<double> Graduator::scaleNonlinearityForward() const {
    return m_forward.scaleNonlinearity();
}

std::vector<double> Graduator::scaleNonlinearityBackward() const {
    return m_backward.scaleNonlinearity();
}

std::vector<std::size_t> Graduator::anglesCountForward() const {
    return m_forward.anglesCount();
}

std::vector<std::size_t> Graduator::anglesCountBackward() const {
    return m_backward.anglesCount();
}

std::vector<const DebugData*> Graduator::allDebugDataForward() const {
    return m_forward.allDebugData();
}

std::vector<const DebugData*> Graduator::allDebugDataBackward() const {
    return m_backward.allDebugData();
}

void Graduator::clear() {
    m_forward.clear();
    m_backward.clear();
}

