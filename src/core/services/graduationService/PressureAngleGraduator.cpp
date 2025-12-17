#include "PressureAngleGraduator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace grad;

PressureAngleGraduator::PressureAngleGraduator(const std::vector<std::pair<double, double>>& pressureSeries)
    : m_pressureSeries_(pressureSeries)
    , m_pressureWindow_(5.0)
    , m_minPoints_(7)
    , m_loessFrac_(0.3)
{
}

// ---------------------- Public API ----------------------

void PressureAngleGraduator::addAngleSample(double time, double angle) {
    m_angleSeries_.emplace_back(time, angle);
}

void PressureAngleGraduator::setNodePressures(const std::vector<double>& pressures) {
    m_nodePressures_ = pressures;
}

void PressureAngleGraduator::setPressureWindow(double window) {
    m_pressureWindow_ = window;
}

void PressureAngleGraduator::setMinPoints(std::size_t minPoints) {
    m_minPoints_ = minPoints;
}

void PressureAngleGraduator::setLoessFrac(double frac) {
    m_loessFrac_ = frac;
}

const std::vector<NodeResult>& PressureAngleGraduator::graduate() const {
    if (m_lastAnglesCount_ == m_angleSeries_.size()) return m_results_;
    if (m_pressureSeries_.size() < 2 || m_angleSeries_.size() < 2) {
        // throw std::runtime_error("Not enough samples to graduate.");
        return m_results_;
    }
    if (m_nodePressures_.empty()) {
        // throw std::runtime_error("Node pressures are not configured.");
        return m_results_;
    }
    m_debugData_ = DebugData{};

    // 1) sort by time
    std::vector<std::pair<double,double>> tp = m_pressureSeries_;
    std::vector<std::pair<double,double>> ta = m_angleSeries_;

    std::sort(tp.begin(), tp.end(),
              [](auto const& a, auto const& b){ return a.first < b.first; });
    std::sort(ta.begin(), ta.end(),
              [](auto const& a, auto const& b){ return a.first < b.first; });

    auto tpTimes    = extractTimes(tp);
    auto tpValues   = extractValues(tp);
    auto taTimes    = extractTimes(ta);
    auto taValues   = extractValues(ta);

    // 2) common time interval
    double tMin = std::max(tpTimes.front(), taTimes.front());
    double tMax = std::min(tpTimes.back(), taTimes.back());
    if (tMax <= tMin) {
        return m_results_;
    }

    // 3) dt from minimum positive time step
    double dtP = computeDt(tpTimes);
    double dtA = computeDt(taTimes);
    double dt  = std::min(dtP, dtA);

    auto timeGrid = buildUniformTimeGrid(tMin, tMax, dt);

    // 4) interpolate to uniform grid
    auto pUniform = interpolate(tpTimes, tpValues, timeGrid);
    auto aUniform = interpolate(taTimes, taValues, timeGrid);

    // 5) build (pressure, angle) pairs and sort by pressure
    std::vector<std::pair<double,double>> pa;
    pa.reserve(timeGrid.size());
    for (std::size_t i = 0; i < timeGrid.size(); ++i) {
        pa.emplace_back(pUniform[i], aUniform[i]);
    }

    std::sort(pa.begin(), pa.end(),
              [](auto const& a, auto const& b){ return a.first < b.first; });

    // 6) for each node pressure perform local LOESS
    std::vector<NodeResult> results;
    results.reserve(m_nodePressures_.size());

    for (double nodeP : m_nodePressures_) {
        double low  = nodeP - m_pressureWindow_;
        double high = nodeP + m_pressureWindow_;

        std::vector<double> localP;
        std::vector<double> localA;
        for (auto const& pr : pa) {
            if (pr.first >= low && pr.first <= high) {
                localP.push_back(pr.first);
                localA.push_back(pr.second);
            }
        }

        if (localP.size() < m_minPoints_) {
            // Not enough points
            results.push_back(NodeResult{
                nodeP,
                std::numeric_limits<double>::quiet_NaN(),
                false
            });
            continue;
        }

        // sort local by pressure (just in case)
        std::vector<std::size_t> idx(localP.size());
        for (std::size_t i = 0; i < idx.size(); ++i) idx[i] = i;
        std::sort(idx.begin(), idx.end(),
                  [&](std::size_t a, std::size_t b){
                      return localP[a] < localP[b];
                  });

        std::vector<double> sortedP(localP.size());
        std::vector<double> sortedA(localA.size());
        for (std::size_t i = 0; i < idx.size(); ++i) {
            sortedP[i] = localP[idx[i]];
            sortedA[i] = localA[idx[i]];
        }

        // выбрать 2 ближайшие точки к nodeP
        std::vector<double> winP;
        std::vector<double> winA;

        for (std::size_t i = 0; i + 1 < sortedP.size(); ++i) {
            if (nodeP >= sortedP[i] && nodeP <= sortedP[i + 1]) {
                winP = { sortedP[i], sortedP[i + 1] };
                winA = { sortedA[i], sortedA[i + 1] };
                break;
            }
        }

        // если nodeP вне диапазона — берём крайние две
        if (winP.empty()) {
            winP = { sortedP[sortedP.size() - 2], sortedP.back() };
            winA = { sortedA[sortedA.size() - 2], sortedA.back() };
        }

        double estAngle = linearRegressionAt(winP, winA, nodeP);


        results.push_back(NodeResult{
            nodeP,
            estAngle,
            true
        });

        DebugData::LocalWindow lw;
        lw.nodePressure = nodeP;
        lw.localPressure = sortedP;
        lw.localAngle = sortedA;
        // lw.smoothAngle = smoothA;

        m_debugData_.windows.push_back(std::move(lw));
    }

    m_results_ = results;
    m_lastAnglesCount_ = m_angleSeries_.size();

    return m_results_;
}

double PressureAngleGraduator::scaleAngleRange() const {
     if (m_results_.size() < 2) return 0.0;
    return m_results_.back().angle - m_results_.front().angle;
}

double PressureAngleGraduator::scaleNonlinearity() const {
    // Соберём все валидные углы
    std::vector<double> angles;
    angles.reserve(m_results_.size());
    for (auto const& r : m_results_) {
        if (r.valid && std::isfinite(r.angle)) {
            angles.push_back(r.angle);
        }
    }

    const std::size_t n = angles.size();
    if (n < 2) return 0.0;

    // Среднее приращение угла
    const double avrDelta = (angles.back() - angles.front()) / double(n - 1);

    // Максимальное отклонение Δᵢ от среднего
    double maxD = 0.0;
    for (std::size_t i = 0; i + 1 < n; ++i) {
        double d = angles[i + 1] - angles[i];
        double dev = std::fabs(d - avrDelta);
        if (dev > maxD) {
            maxD = dev;
        }
    }

    if (std::fabs(avrDelta) < 1e-15) {
        return 0.0; // избежание деления на ноль
    }

    return (maxD / std::fabs(avrDelta)) * 100.0;
}


std::size_t PressureAngleGraduator::anglesCount() const {
    return m_angleSeries_.size();
}

double PressureAngleGraduator::currentAngle() const {
    return m_angleSeries_.empty() ? 0.0 : m_angleSeries_.back().second;
}


void PressureAngleGraduator::clear() {
    m_angleSeries_.clear();
    m_results_.clear();
    m_lastAnglesCount_ = 0;
    m_debugData_ = DebugData{};
}

// ---------------------- Helpers ----------------------

std::vector<double> PressureAngleGraduator::extractTimes(
    const std::vector<std::pair<double,double>>& series)
{
    std::vector<double> out;
    out.reserve(series.size());
    for (auto const& p : series) {
        out.push_back(p.first);
    }
    return out;
}

std::vector<double> PressureAngleGraduator::extractValues(
    const std::vector<std::pair<double,double>>& series)
{
    std::vector<double> out;
    out.reserve(series.size());
    for (auto const& p : series) {
        out.push_back(p.second);
    }
    return out;
}

double PressureAngleGraduator::computeDt(const std::vector<double>& times) {
    if (times.size() < 2) {
        throw std::runtime_error("Not enough points to compute dt.");
    }
    double dt = std::numeric_limits<double>::max();
    for (std::size_t i = 1; i < times.size(); ++i) {
        double d = times[i] - times[i-1];
        if (d > 0.0 && d < dt) {
            dt = d;
        }
    }
    if (!std::isfinite(dt) || dt <= 0.0) {
        throw std::runtime_error("Failed to compute positive dt.");
    }
    return dt;
}

std::vector<double> PressureAngleGraduator::buildUniformTimeGrid(
    double tMin, double tMax, double dt)
{
    std::vector<double> grid;
    std::size_t n = static_cast<std::size_t>((tMax - tMin) / dt);
    grid.reserve(n + 1);
    double t = tMin;
    while (t <= tMax) {
        grid.push_back(t);
        t += dt;
    }
    if (grid.back() < tMax) {
        grid.push_back(tMax);
    }
    return grid;
}

std::vector<double> PressureAngleGraduator::interpolate(
    const std::vector<double>& times,
    const std::vector<double>& values,
    const std::vector<double>& newTimes)
{
    if (times.size() != values.size()) {
        throw std::runtime_error("Times and values size mismatch.");
    }
    if (times.empty()) {
        throw std::runtime_error("Empty time series.");
    }

    std::vector<double> out;
    out.reserve(newTimes.size());

    std::size_t j = 0; // index in original times
    for (double t : newTimes) {
        // clamp to edges
        if (t <= times.front()) {
            out.push_back(values.front());
            continue;
        }
        if (t >= times.back()) {
            out.push_back(values.back());
            continue;
        }

        // move j until times[j] <= t < times[j+1]
        while (j + 1 < times.size() && times[j+1] < t) {
            ++j;
        }
        // now t in [times[j], times[j+1]] or [times[j], times[j+1]) with next >= t
        double t0 = times[j];
        double t1 = times[j+1];
        double v0 = values[j];
        double v1 = values[j+1];

        double alpha = (t - t0) / (t1 - t0);
        out.push_back(v0 + alpha * (v1 - v0));
    }

    return out;
}

double PressureAngleGraduator::linearRegressionAt(
    const std::vector<double>& x,
    const std::vector<double>& y,
    double xq)
{
    const std::size_t n = x.size();
    if (n < 2 || n != y.size()) {
        throw std::runtime_error("linearRegressionAt: invalid input");
    }

    double Sx = 0.0, Sy = 0.0, Sxx = 0.0, Sxy = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        Sx  += x[i];
        Sy  += y[i];
        Sxx += x[i] * x[i];
        Sxy += x[i] * y[i];
    }

    double denom = n * Sxx - Sx * Sx;
    if (std::fabs(denom) < 1e-12) {
        return Sy / n; // fallback: среднее
    }

    double b = (n * Sxy - Sx * Sy) / denom;
    double a = (Sy - b * Sx) / n;

    return a + b * xq;
}



// Linear interpolation + edge clamping
double PressureAngleGraduator::interpolateAt(
    const std::vector<double>& x,
    const std::vector<double>& y,
    double xq)
{
    if (x.empty() || x.size() != y.size()) {
        throw std::runtime_error("Invalid input for interpolateAt.");
    }
    if (xq <= x.front()) {
        return y.front();
    }
    if (xq >= x.back()) {
        return y.back();
    }

    // find segment
    // since x is sorted:
    auto it = std::upper_bound(x.begin(), x.end(), xq);
    std::size_t idx1 = static_cast<std::size_t>(it - x.begin());
    std::size_t idx0 = idx1 - 1;

    double x0 = x[idx0];
    double x1 = x[idx1];
    double y0 = y[idx0];
    double y1 = y[idx1];

    double alpha = (xq - x0) / (x1 - x0);
    return y0 + alpha * (y1 - y0);
}

