#ifndef GRADUATOR_GRADUATOR_H
#define GRADUATOR_GRADUATOR_H
#include <mutex>
#include <vector>
#include <map>
using namespace std;
struct time_point_t {
    time_point_t(double tx, double ty) : x(tx), y(ty) {
    }
    double x;
    double y;
};
class Graduator {
public:
    explicit Graduator();
    void setPressureNodes(const std::vector<double> &pressureNodes);
    void pushPressure(double t, double pressure);
    void pushAngle(int i, double t, double angle);
    std::vector<std::vector<double> > graduate(int p_window, int a_window);
    void clear();
private:
    int searchIndexOfPressure(double targetPressure) const;
    int searchIndexOfAngle(int camIdx, double targetTime) const;
private:
    std::vector<double> m_pressureNodes;
    std::vector<time_point_t> m_pressureData;
    std::vector<std::vector<time_point_t> > m_angleData;
};
#endif //GRADUATOR_GRADUATOR_H