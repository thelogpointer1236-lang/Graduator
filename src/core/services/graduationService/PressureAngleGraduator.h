#ifndef PRESSURE_ANGLE_CALIBRATOR_H
#define PRESSURE_ANGLE_CALIBRATOR_H

#include <vector>
#include <utility>

#include "core/types/Grad.h"

namespace grad {

    class PressureAngleGraduator {
    public:
        explicit PressureAngleGraduator(const std::vector<std::pair<double, double>>& pressureSeries);

        // Add angle
        void addAngleSample(double time, double angle);

        // Configuration
        void setNodePressures(const std::vector<double>& pressures);
        void setPressureWindow(double window);
        void setMinPoints(std::size_t minPoints);
        void setLoessFrac(double frac);

        // Main API: compute pressure-angle table
        const std::vector<NodeResult>& graduate() const;
        double scaleAngleRange() const;
        double scaleNonlinearity() const;
        std::size_t anglesCount() const;
        double currentAngle() const;
        const DebugData& debugData() const { return m_debugData_; }
        void clear();

    private:
        const std::vector<std::pair<double, double>>& m_pressureSeries_; // (time, pressure)
        std::vector<std::pair<double, double>> m_angleSeries_;    // (time, angle)

        std::vector<double> m_nodePressures_;
        double              m_pressureWindow_;
        std::size_t         m_minPoints_;
        double              m_loessFrac_;
        mutable DebugData   m_debugData_;

        mutable std::vector<NodeResult> m_results_;
        mutable std::size_t m_lastAnglesCount_ = 0;

        // Helpers
        static std::vector<double> extractTimes(const std::vector<std::pair<double,double>>& series);
        static std::vector<double> extractValues(const std::vector<std::pair<double,double>>& series);
        static double              computeDt(const std::vector<double>& times);
        static std::vector<double> buildUniformTimeGrid(double tMin, double tMax, double dt);

        static std::vector<double> interpolate(
            const std::vector<double>& times,
            const std::vector<double>& values,
            const std::vector<double>& newTimes
        );


        static double linearRegressionAt(
            const std::vector<double>& x,
            const std::vector<double>& y,
            double xq
        );


        // Simple linear interpolation for a single query
        static double interpolateAt(
            const std::vector<double>& x,
            const std::vector<double>& y,
            double xq
        );
    };

} // namespace grad

#endif // PRESSURE_ANGLE_CALIBRATOR_H