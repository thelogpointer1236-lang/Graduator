#ifndef GRADUATORTEST_BATCHCALIBRATOR_H
#define GRADUATORTEST_BATCHCALIBRATOR_H
#include <vector>
#include <cstddef>
#include "PressureAngleGraduator.h"

namespace grad {
    class BatchGraduator {
    public:
        explicit BatchGraduator(int cameraCount);
        ~BatchGraduator();

        void addPressureSample(double time, double pressure);
        void addAngleSample(int cameraIndex, double time, double angle);

        void setNodePressures(const std::vector<double>& pressures);
        void setPressureWindow(double window);
        void setMinPoints(std::size_t minPoints);
        void setLoessFrac(double frac);

        // Main API:
        std::vector<std::vector<NodeResult>> graduate() const;
        std::vector<double> scaleAngleRange() const;
        std::vector<double> scaleNonlinearity() const;
        std::vector<std::size_t> anglesCount() const;
        std::vector<const DebugData*> allDebugData() const;
        void clear();


    private:
        std::vector<PressureAngleGraduator> m_calibrators;
        std::vector<std::pair<double, double>> m_pressureSeries;
    };
}

#endif //GRADUATORTEST_BATCHCALIBRATOR_H