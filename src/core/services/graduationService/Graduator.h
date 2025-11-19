#ifndef GRADUATORTEST_CALIBRATOR_H
#define GRADUATORTEST_CALIBRATOR_H
#include "BatchGraduator.h"

namespace grad {
    class Graduator {
    public:
        explicit Graduator(int cameraCount);

        void switchToForward();
        void switchToBackward();
        bool isForward() const;

        // Proxy API (те же методы, что и у BatchCalibrator)
        void addPressureSample(double time, double pressure);
        void addAngleSample(int cameraIndex, double time, double angle);

        void setNodePressures(const std::vector<double>& pressures);
        void setPressureWindow(double window);
        void setMinPoints(std::size_t minPoints);
        void setLoessFrac(double frac);

        // --------- Разделённый API ---------
        std::vector<std::vector<NodeResult>> graduateForward() const;
        std::vector<std::vector<NodeResult>> graduateBackward() const;

        std::vector<double> scaleAngleRangeForward() const;
        std::vector<double> scaleAngleRangeBackward() const;

        std::vector<double> scaleNonlinearityForward() const;
        std::vector<double> scaleNonlinearityBackward() const;

        std::vector<std::size_t> anglesCountForward() const;
        std::vector<std::size_t> anglesCountBackward() const;

        const std::vector<double>& currentAngles() const;

        std::vector<const DebugData*> allDebugDataForward() const;
        std::vector<const DebugData*> allDebugDataBackward() const;

        void clear();

    private:
        bool m_isForward = true;
        BatchGraduator m_forward;
        BatchGraduator m_backward;
        std::vector<double> m_lastAngles;
        int m_cameraCount = 0;

        BatchGraduator& active();
        const BatchGraduator& active() const;
    };
} // namespace grad


#endif //GRADUATORTEST_CALIBRATOR_H