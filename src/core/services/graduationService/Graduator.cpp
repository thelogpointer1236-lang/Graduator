#include "graduator.h"
#include <iostream>
// аппроксимация параболой y = a*x^2 + b*x + c по трём точкам
template<typename It>
void fitParabola(It begin, It end, double &a, double &b, double &c) {
    size_t n = distance(begin, end);
    if (n < 3) throw invalid_argument("Нужно минимум 3 точки");
    double Sx = 0, Sx2 = 0, Sx3 = 0, Sx4 = 0;
    double Sy = 0, Sxy = 0, Sx2y = 0;
    for (auto it = begin; it != end; ++it) {
        double x = it->x;
        double y = it->y;
        double x2 = x * x;
        Sx += x;
        Sx2 += x2;
        Sx3 += x2 * x;
        Sx4 += x2 * x2;
        Sy += y;
        Sxy += x * y;
        Sx2y += x2 * y;
    }
    // Система нормальных уравнений:
    // | Sx4  Sx3  Sx2 |   | a |   | Sx2y |
    // | Sx3  Sx2  Sx  | * | b | = | Sxy  |
    // | Sx2  Sx   n   |   | c |   | Sy   |
    double M[3][4] = {
        {Sx4, Sx3, Sx2, Sx2y},
        {Sx3, Sx2, Sx, Sxy},
        {Sx2, Sx, (double) n, Sy}
    };
    // Прямая и обратная подстановка (Гаусс)
    for (int i = 0; i < 3; ++i) {
        // Нормировка ведущего элемента
        double pivot = M[i][i];
        if (fabs(pivot) < 1e-12) {
            std::cerr << "For i=" << i << "\n";
            std::cerr << "Matrix:\n";
            for (int r = 0; r < 3; ++r) {
                std::cerr << M[r][0] << "  " << M[r][1] << "  " << M[r][2] << "  |  " << M[r][3] << "\n";
            }
            throw runtime_error("Вырожденная система");
        }
        for (int j = i; j < 4; ++j) M[i][j] /= pivot;
        // Обнуление ниже
        for (int k = i + 1; k < 3; ++k) {
            double factor = M[k][i];
            for (int j = i; j < 4; ++j)
                M[k][j] -= factor * M[i][j];
        }
    }
    // Обратный ход
    for (int i = 2; i >= 0; --i) {
        for (int k = i - 1; k >= 0; --k) {
            double factor = M[k][i];
            M[k][3] -= factor * M[i][3];
            M[k][i] = 0;
        }
    }
    a = M[0][3];
    b = M[1][3];
    c = M[2][3];
}
// решение квадратного уравнения
inline vector<double> solveQuadratic(double a, double b, double c) {
    vector<double> res;
    const double eps = 1e-12;
    if (fabs(a) < eps) {
        // линейное уравнение
        if (fabs(b) < eps) return res;
        res.push_back(-c / b);
        return res;
    }
    double D = b * b - 4 * a * c;
    if (D < -eps) return res;
    if (D < 0) D = 0; // исправляем погрешность вычислений
    double sqrtD = sqrt(D);
    res.push_back((-b + sqrtD) / (2 * a));
    if (D > eps) res.push_back((-b - sqrtD) / (2 * a));
    return res;
}
template<typename It1, typename It2>
double findAbyP(It1 tp_begin, It1 tp_end,
                It2 ta_begin, It2 ta_end,
                double p_target, double t_guess) {
    double ap, bp, cp;
    double aa, ba, ca;
    fitParabola(tp_begin, tp_end, ap, bp, cp);
    fitParabola(ta_begin, ta_end, aa, ba, ca);
    // решаем ap*x^2 + bp*x + cp = Ptarget
    std::vector<double> roots = solveQuadratic(ap, bp, cp - p_target);
    if (roots.empty())
        throw std::runtime_error("Нет решения для tP");
    // выбираем корень, ближайший к t_guess
    double tP = roots[0];
    double best_diff = std::abs(roots[0] - t_guess);
    for (double r: roots) {
        double diff = std::abs(r - t_guess);
        if (diff < best_diff) {
            best_diff = diff;
            tP = r;
        }
    }
    return aa * tP * tP + ba * tP + ca;
}
Graduator::Graduator() {
    m_angleData.resize(8);
    m_pressureData.reserve(60 * 5 * 15); // 5 минут по 15 Гц
    m_angleData.reserve(60 * 5 * 30); // 5 минут по 30 Гц
}
void Graduator::setPressureNodes(const std::vector<double> &pressureNodes) {
    m_pressureNodes = pressureNodes;
}
void Graduator::pushPressure(double t, double pressure) {
    m_pressureData.emplace_back(t, pressure);
}
void Graduator::pushAngle(int i, double t, double angle) {
    if (i <= 1 || i >= 8) return;
    m_angleData[i - 1].emplace_back(t, angle);
}
std::vector<std::vector<double> > Graduator::graduate(int p_window, int a_window) {
    std::vector<std::vector<double> > scaleGrid;
    scaleGrid.resize(m_pressureNodes.size());
    int i = 0;
    for (double P: m_pressureNodes) {
        scaleGrid[i].resize(8, 0.0);
        for (int c = 0; c < 8; c++) {
            const auto &angleData = m_angleData[c];
            int t_idx = searchIndexOfPressure(P);
            if (t_idx < 0) continue;
            int a_idx = searchIndexOfAngle(c, m_pressureData[t_idx].x);
            if (a_idx < 0) continue;
            try {
                int p_start = std::fmax(0, t_idx - p_window);
                int p_end = std::fmin(static_cast<int>(m_pressureData.size()), t_idx + p_window);
                int a_start = std::fmax(0, a_idx - a_window);
                int a_end = std::fmin(static_cast<int>(angleData.size()), a_idx + a_window);
                double angle = findAbyP(
                    m_pressureData.begin() + p_start,
                    m_pressureData.begin() + p_end,
                    angleData.begin() + a_start,
                    angleData.begin() + a_end,
                    P, m_pressureData[t_idx].x
                );
                scaleGrid[i][c] = angle;
            } catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error" << std::endl;
            }
        }
        i++;
    }
    return scaleGrid;
}
void Graduator::clear() {
    m_pressureData.clear();
    for (auto &vec: m_angleData) {
        vec.clear();
    }
}
int Graduator::searchIndexOfPressure(double targetPressure) const {
    if (m_pressureData.empty()) return -1;
    double minDiff = std::numeric_limits<double>::max();
    int bestIdx = -1;
    for (size_t i = 0; i < m_pressureData.size(); ++i) {
        double diff = std::abs(m_pressureData[i].y - targetPressure);
        if (diff < minDiff) {
            minDiff = diff;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}
int Graduator::searchIndexOfAngle(int camIdx, double targetTime) const {
    if (camIdx < 0 || camIdx >= 8) return -1;
    const auto &angleData = m_angleData[camIdx];
    if (angleData.empty()) return -1;
    double minDiff = std::numeric_limits<double>::max();
    int bestIdx = -1;
    for (size_t i = 0; i < angleData.size(); ++i) {
        double diff = std::abs(angleData[i].x - targetTime);
        if (diff < minDiff) {
            minDiff = diff;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}