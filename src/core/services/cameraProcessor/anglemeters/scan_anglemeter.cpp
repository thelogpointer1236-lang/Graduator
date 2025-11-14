#ifndef TEST_SCAN_COLS_H
#define TEST_SCAN_COLS_H
#include <cmath>
#include <cstdint>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <vector>
#include "scan_anglemeter.h"
#define M_PI 3.14159265358979323846
struct pos_t {
    int x;
    int y;
};
struct scan_t {
    pos_t posDifMin;
    pos_t posDifMax;
    bool valid;
};
struct rgb_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};
inline uint8_t brightness(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint8_t>(0.299f * r + 0.587f * g + 0.114f * b);
}
inline bool isGray(uint8_t r, uint8_t g, uint8_t b, float eps) {
    float avr = (r + g + b) / 3.0f;
    float dx = r - avr;
    float dy = g - avr;
    float dz = b - avr;
    float disp = std::sqrt((dx * dx + dy * dy + dz * dz) / (3.0f * 2.0f));
    return disp < (eps);
}
inline void scanRow(const rgb_t *img, uint16_t width, uint16_t height, uint16_t y, scan_t *scan,
                    uint8_t bright_lim = 70, int max_pairs = 4) {
    struct Edge {
        int x;
        int dBr;
    };
    struct Pair {
        int xMin;
        int xMax;
        float avgLeft;
        float avgRight;
        float avgBright;
    };
    std::vector<Edge> mins, maxs;
    mins.reserve(max_pairs);
    maxs.reserve(max_pairs);
    // 1. Поиск локальных минимумов и максимумов dBr по горизонтали
    for (uint16_t x = 2; x < width - 2; ++x) {
        const rgb_t &p1 = img[y * width + (x - 2)];
        const rgb_t &p2 = img[y * width + x];
        if (!isGray(p1.r, p1.g, p1.b, 10.0f) || !isGray(p2.r, p2.g, p2.b, 10.0f))
            continue;
        int br1 = brightness(p1.r, p1.g, p1.b);
        int br2 = brightness(p2.r, p2.g, p2.b);
        int dBr = br2 - br1;
        if (dBr < -bright_lim) {
            if ((int) mins.size() < max_pairs) mins.push_back({(int) x - 1, dBr});
            else if (dBr < mins.back().dBr) mins.back() = {(int) x - 1, dBr};
        }
        if (dBr > bright_lim) {
            if ((int) maxs.size() < max_pairs) maxs.push_back({(int) x - 1, dBr});
            else if (dBr > maxs.back().dBr) maxs.back() = {(int) x - 1, dBr};
        }
    }
    if (mins.empty() || maxs.empty()) {
        scan->posDifMin = {0, y};
        scan->posDifMax = {0, y};
        scan->valid = false;
        return;
    }
    // 2. Сортировка по силе градиента
    std::sort(mins.begin(), mins.end(), [](auto &a, auto &b) { return a.dBr < b.dBr; });
    std::sort(maxs.begin(), maxs.end(), [](auto &a, auto &b) { return a.dBr > b.dBr; });
    // 3. Подбор всех подходящих пар
    auto avgRegion = [&](int startX, int endX) {
        if (startX < 0 || endX >= width) return 0.0f;
        int s = 0, c = 0;
        for (int x = startX; x <= endX; ++x) {
            const rgb_t &p = img[y * width + x];
            s += brightness(p.r, p.g, p.b);
            c++;
        }
        return c > 0 ? (float) s / (float) c : 0.0f;
    };
    std::vector<Pair> candidates;
    for (auto &minE: mins) {
        for (auto &maxE: maxs) {
            int dx = maxE.x - minE.x;
            if (dx > 10 && dx < 50) {
                // Средняя яркость между ними
                int sum = 0, cnt = 0;
                for (int x = minE.x; x <= maxE.x; ++x) {
                    const rgb_t &p = img[y * width + x];
                    sum += brightness(p.r, p.g, p.b);
                    cnt++;
                }
                float avgBright = (float) sum / (float) cnt;
                if (avgBright >= bright_lim) continue;
                float avgLeft = avgRegion(minE.x - 16, minE.x - 1);
                float avgRight = avgRegion(maxE.x + 1, maxE.x + 16);
                if (avgLeft < 200.0f || avgRight < 200.0f) continue;
                candidates.push_back({minE.x, maxE.x, avgLeft, avgRight, avgBright});
            }
        }
    }
    if (candidates.empty()) {
        scan->posDifMin = {0, y};
        scan->posDifMax = {0, y};
        scan->valid = false;
        return;
    }
    // 4. Выбор пары с максимальной суммарной яркостью по краям
    auto best = std::max_element(candidates.begin(), candidates.end(),
                                 [](const Pair &a, const Pair &b) {
                                     return (a.avgLeft + a.avgRight) < (b.avgLeft + b.avgRight);
                                 });
    scan->posDifMin = {(uint16_t) best->xMin, y};
    scan->posDifMax = {(uint16_t) best->xMax, y};
    scan->valid = true;
}
inline void scanCol(const rgb_t *img, uint16_t width, uint16_t height, uint16_t x, scan_t *scan,
                    uint8_t bright_lim = 70, int max_pairs = 4) {
    struct Edge {
        int y;
        int dBr;
    };
    struct Pair {
        int yMin;
        int yMax;
        float avgUp;
        float avgDown;
        float avgBright;
    };
    std::vector<Edge> mins, maxs;
    mins.reserve(max_pairs);
    maxs.reserve(max_pairs);
    // 1. Поиск локальных минимумов и максимумов dBr по вертикали
    for (uint16_t y = 2; y < height - 2; ++y) {
        const rgb_t &p1 = img[(y - 2) * width + x];
        const rgb_t &p2 = img[y * width + x];
        if (!isGray(p1.r, p1.g, p1.b, 10.0f) || !isGray(p2.r, p2.g, p2.b, 10.0f))
            continue;
        int br1 = brightness(p1.r, p1.g, p1.b);
        int br2 = brightness(p2.r, p2.g, p2.b);
        int dBr = br2 - br1;
        if (dBr < -bright_lim) {
            if ((int) mins.size() < max_pairs) mins.push_back({(int) y - 1, dBr});
            else if (dBr < mins.back().dBr) mins.back() = {(int) y - 1, dBr};
        }
        if (dBr > bright_lim) {
            if ((int) maxs.size() < max_pairs) maxs.push_back({(int) y - 1, dBr});
            else if (dBr > maxs.back().dBr) maxs.back() = {(int) y - 1, dBr};
        }
    }
    if (mins.empty() || maxs.empty()) {
        scan->posDifMin = {x, 0};
        scan->posDifMax = {x, 0};
        scan->valid = false;
        return;
    }
    // 2. Сортировка по силе градиента
    std::sort(mins.begin(), mins.end(), [](auto &a, auto &b) { return a.dBr < b.dBr; });
    std::sort(maxs.begin(), maxs.end(), [](auto &a, auto &b) { return a.dBr > b.dBr; });
    // 3. Подбор всех подходящих пар
    auto avgRegion = [&](int startY, int endY) {
        if (startY < 0 || endY >= height) return 0.0f;
        int s = 0, c = 0;
        for (int y = startY; y <= endY; ++y) {
            const rgb_t &p = img[y * width + x];
            s += brightness(p.r, p.g, p.b);
            c++;
        }
        return c > 0 ? (float) s / (float) c : 0.0f;
    };
    std::vector<Pair> candidates;
    for (auto &minE: mins) {
        for (auto &maxE: maxs) {
            int dy = maxE.y - minE.y;
            if (dy > 10 && dy < 50) {
                // Средняя яркость между ними
                int sum = 0, cnt = 0;
                for (int y = minE.y; y <= maxE.y; ++y) {
                    const rgb_t &p = img[y * width + x];
                    sum += brightness(p.r, p.g, p.b);
                    cnt++;
                }
                float avgBright = (float) sum / (float) cnt;
                if (avgBright >= bright_lim) continue;
                float avgUp = avgRegion(minE.y - 16, minE.y - 1);
                float avgDown = avgRegion(maxE.y + 1, maxE.y + 16);
                if (avgUp < 200.0f || avgDown < 200.0f) continue;
                candidates.push_back({minE.y, maxE.y, avgUp, avgDown, avgBright});
            }
        }
    }
    if (candidates.empty()) {
        scan->posDifMin = {x, 0};
        scan->posDifMax = {x, 0};
        scan->valid = false;
        return;
    }
    // 4. Выбор пары с максимальной суммарной яркостью по краям
    auto best = std::max_element(candidates.begin(), candidates.end(),
                                 [](const Pair &a, const Pair &b) {
                                     return (a.avgUp + a.avgDown) < (b.avgUp + b.avgDown);
                                 });
    scan->posDifMin = {x, (uint16_t) best->yMin};
    scan->posDifMax = {x, (uint16_t) best->yMax};
    scan->valid = true;
}
inline void scanRows(const rgb_t *img, uint16_t width, uint16_t height, scan_t *scans, uint16_t *cnt1, uint16_t *cnt2) {
    if (cnt1 == nullptr || cnt2 == nullptr) return;
    for (uint16_t y = 0; y < height; ++y) {
        scanRow(img, width, height, y, &scans[y]);
        if (y < height / 2) *cnt1 += scans[y].valid;
        else *cnt2 += scans[y].valid;
    }
}
inline void scanCols(const rgb_t *img, uint16_t width, uint16_t height, scan_t *scans, uint16_t *cnt1, uint16_t *cnt2) {
    if (cnt1 == nullptr || cnt2 == nullptr) return;
    for (uint16_t x = 0; x < width; ++x) {
        scanCol(img, width, height, x, &scans[x]);
        if (x < width / 2) *cnt1 += scans[x].valid;
        else *cnt2 += scans[x].valid;
    }
}
inline void selectValidHalfPoints(uint16_t width, uint16_t height,
                                  const scan_t *cscans, uint16_t ccnt1, uint16_t ccnt2,
                                  const scan_t *rscans, uint16_t rcnt1, uint16_t rcnt2,
                                  pos_t *p1, pos_t *p2, uint16_t *cnt) {
    *cnt = 0;
    bool useCols = (ccnt1 + ccnt2) > (rcnt1 + rcnt2);
    bool useFirstHalf = useCols ? (ccnt1 > ccnt2) : (rcnt1 > rcnt2);
    const scan_t *src = useCols ? cscans : rscans;
    uint16_t limit = useCols ? width : height;
    uint16_t from = useFirstHalf ? 0 : limit - 1;
    uint16_t to = useFirstHalf ? limit / 2 : limit / 2 - 1;
    int16_t direct = useFirstHalf ? 1 : -1;
    uint16_t i = from;
    while ((useFirstHalf && i < to) || (!useFirstHalf && i > to)) {
        if (src[i].valid) {
            // Логика направления совпадает со старым кодом
            if (useCols) {
                if (direct > 0) {
                    p1[*cnt] = src[i].posDifMin;
                    p2[*cnt] = src[i].posDifMax;
                } else {
                    p1[*cnt] = src[i].posDifMax;
                    p2[*cnt] = src[i].posDifMin;
                }
            } else {
                if (direct > 0) {
                    p1[*cnt] = src[i].posDifMax;
                    p2[*cnt] = src[i].posDifMin;
                } else {
                    p1[*cnt] = src[i].posDifMin;
                    p2[*cnt] = src[i].posDifMax;
                }
            }
            (*cnt)++;
        }
        i += direct;
    }
}
// Подсчёт количества точек, лежащих на линии (по нормали и опорной точке)
static inline uint16_t cntOnLine(
    const pos_t *points,
    uint16_t cnt,
    float nx,
    float ny,
    const pos_t &ref,
    float eps
) {
    uint16_t count = 0;
    for (uint16_t i = 0; i < cnt; i++) {
        float vx = static_cast<float>(points[i].x) - static_cast<float>(ref.x);
        float vy = static_cast<float>(points[i].y) - static_cast<float>(ref.y);
        float r = std::fabs(vx * nx + vy * ny) / std::sqrtf(nx * nx + ny * ny);
        if (r < eps)
            count++;
    }
    return count;
}
// Основная функция фильтрации
inline void weedOutPoints(const pos_t *p, uint16_t cnt, pos_t *wp, uint16_t *wcnt) {
    if (cnt <= 20 || !p || !wp || !wcnt) {
        *wcnt = 0;
        return;
    }
    const float eps = 1.5f;
    const float r_min = 3.0f;
    const float alpha = 0.6f;
    const float beta = 0.8f;
    uint16_t i1max = 0;
    uint16_t i2max = cnt - 1;
    const pos_t *p1 = &p[i1max];
    const pos_t *p2 = &p[i2max];
    float nx = static_cast<float>(p2->y) - static_cast<float>(p1->y);
    float ny = static_cast<float>(p1->x) - static_cast<float>(p2->x);
    uint16_t maxCnt = cntOnLine(p, cnt, nx, ny, *p1, eps);
    uint16_t mid1 = (cnt / 3u) * 2u;
    float alphaCoef = alpha * static_cast<float>(cnt);
    float betaCoef = beta * static_cast<float>(cnt);
    for (uint16_t i1 = 0; i1 <= mid1 && maxCnt < alphaCoef; i1++) {
        p1 = &p[i1];
        uint16_t mid2 = i1 + cnt / 3u;
        if (mid2 >= cnt) mid2 = cnt - 1;
        for (uint16_t i2 = cnt - 2; i2 >= mid2 && maxCnt < betaCoef; i2--) {
            p2 = &p[i2];
            nx = static_cast<float>(p2->y) - static_cast<float>(p1->y);
            ny = static_cast<float>(p1->x) - static_cast<float>(p2->x);
            uint16_t c = cntOnLine(p, cnt, nx, ny, *p1, eps);
            if (c > maxCnt) {
                maxCnt = c;
                i1max = i1;
                i2max = i2;
            }
            if (i2 == 0) break;
        }
    }
    // повторное вычисление лучшей линии
    p1 = &p[i1max];
    p2 = &p[i2max];
    nx = static_cast<float>(p2->y) - static_cast<float>(p1->y);
    ny = static_cast<float>(p1->x) - static_cast<float>(p2->x);
    // фильтрация точек
    uint16_t outCount = 0;
    for (uint16_t i = 0; i < cnt; i++) {
        float vx = static_cast<float>(p[i].x) - static_cast<float>(p1->x);
        float vy = static_cast<float>(p[i].y) - static_cast<float>(p1->y);
        float r = std::fabs(vx * nx + vy * ny) / std::sqrtf(nx * nx + ny * ny);
        if (r <= r_min) {
            wp[outCount++] = p[i];
        }
    }
    *wcnt = outCount;
}
//-----------------------------------------
// Вспомогательные структуры и функции
//-----------------------------------------
struct histogram_t {
    static constexpr uint32_t SIZE = 360; // гистограмма по углу
    std::vector<uint32_t> data;
    histogram_t() : data(SIZE, 0) {
    }
    void clear() { std::fill(data.begin(), data.end(), 0); }
    uint32_t &operator[](uint32_t idx) { return data[idx % SIZE]; }
    const uint32_t &operator[](uint32_t idx) const { return data[idx % SIZE]; }
    uint32_t maxIndex(uint32_t *maxValue = nullptr) const {
        uint32_t idx = 0;
        uint32_t mv = 0;
        for (uint32_t i = 0; i < SIZE; i++) {
            if (data[i] > mv) {
                mv = data[i];
                idx = i;
            }
        }
        if (maxValue) *maxValue = mv;
        return idx;
    }
    uint32_t sumRange(int32_t center, uint32_t radius) const {
        uint32_t sum = 0;
        for (int32_t i = -static_cast<int32_t>(radius); i <= static_cast<int32_t>(radius); i++) {
            int32_t idx = (center + i + SIZE) % SIZE;
            sum += data[idx];
        }
        return sum;
    }
};
inline float deg_to_rad(float deg) { return deg * static_cast<float>(M_PI) / 180.0f; }
inline float rad_to_deg(float rad) { return rad * 180.0f / static_cast<float>(M_PI); }
template<typename T>
inline T clamp(T v, T lo, T hi) { return (v < lo) ? lo : (v > hi ? hi : v); }
//-----------------------------------------
// Главная функция вычисления угла линии
//-----------------------------------------
float calcLine(const pos_t *points, uint32_t n) {
    if (n < 20)
        throw std::runtime_error("pointsSize < 20");
    std::vector<uint32_t> hist(360, 0);
    const float step = n / 3.0f;
    uint32_t maxVal = 0;
    int bestAngle = -1;
    // 1. Построение гистограммы направлений
    for (uint32_t i = 0; i < std::min<uint32_t>(n - 1, std::roundf(2.0f * step) - 1); ++i) {
        const pos_t &pi = points[i];
        for (uint32_t j = i + static_cast<uint32_t>(step) + 1; j < n; ++j) {
            const pos_t &pj = points[j];
            float dx = pj.x - pi.x;
            float dy = pj.y - pi.y;
            int idx = static_cast<int>(std::roundf(rad_to_deg(std::atan2f(dy, dx))));
            if (idx < 0) idx += 360;
            hist[idx]++;
            if (hist[idx] > maxVal) {
                maxVal = hist[idx];
                bestAngle = idx;
            }
        }
    }
    if (bestAngle < 0)
        throw std::runtime_error("Failed to determine dominant angle");
    // 2. Проверка концентрации
    const int R = 5;
    const float alpha = 0.2f;
    uint32_t cnt = 0;
    for (int i = -R; i <= R; ++i)
        cnt += hist[(bestAngle + i + 360) % 360];
    uint32_t totalPairs = std::accumulate(hist.begin(), hist.end(), 0u);
    if (cnt <= alpha * totalPairs)
        throw std::runtime_error("Line is not sufficiently pronounced");
    // 3. Усреднение углов по синусам/косинусам
    float sx = 0.f, sy = 0.f;
    for (int i = -R; i <= R; ++i) {
        int idx = (bestAngle + i + 360) % 360;
        float w = static_cast<float>(hist[idx]);
        float rad = deg_to_rad(static_cast<float>(idx));
        sx += w * std::cosf(rad);
        sy += w * std::sinf(rad);
    }
    float angle2 = rad_to_deg(std::atan2f(sy, sx));
    if (angle2 < 0.f)
        angle2 += 360.f;
    // 4. Финальная подгонка линии: считаем точки, лежащие близко к линии
    const float dx = std::cosf(deg_to_rad(angle2 - 90.f));
    const float dy = std::sinf(deg_to_rad(angle2 - 90.f));
    // Находим параметр c линии в форме Ax + By + C = 0
    uint32_t maxHits = 0;
    for (uint32_t i = 0; i < n; ++i) {
        const pos_t &p0 = points[i];
        float c = -(dx * p0.x + dy * p0.y);
        uint32_t hits = 0;
        for (uint32_t j = 0; j < n; ++j) {
            const pos_t &pj = points[j];
            float dist = std::fabsf(dx * pj.x + dy * pj.y + c);
            if (dist < 2.f) hits++;
        }
        if (hits > maxHits) {
            maxHits = hits;
        }
    }
    if (maxHits < 5)
        throw std::runtime_error("Failed to determine line anchor region");
    // 5. Нормализация угла
    angle2 = fmod(angle2 + 90.0, 360.0);
    if (angle2 < 0) angle2 += 360.0;
    return angle2;
}
inline float calcAngle(const pos_t *p1, const pos_t *p2, uint32_t cnt, uint32_t width, uint32_t height) {
    float angle1 = calcLine(p1, cnt);
    float angle2 = calcLine(p2, cnt);
    if (std::fabs(angle1 - angle2) > 5.0f) {
        throw std::runtime_error("Failed to determine line angle");
    }
    return (angle1 + angle2) / 2.0f;
}
float scanArrowAngle(const unsigned char *img, unsigned width, unsigned height) {
    std::vector<scan_t> rowScans(height);
    std::vector<scan_t> colScans(width);
    uint16_t rcnt1 = 0, rcnt2 = 0;
    uint16_t ccnt1 = 0, ccnt2 = 0;
    scanRows(reinterpret_cast<const rgb_t *>(img), width, height, rowScans.data(), &rcnt1, &rcnt2);
    scanCols(reinterpret_cast<const rgb_t *>(img), width, height, colScans.data(), &ccnt1, &ccnt2);
    std::vector<pos_t> p1(std::max(width, height));
    std::vector<pos_t> p2(std::max(width, height));
    uint16_t cnt = 0;
    selectValidHalfPoints(width, height,
                          colScans.data(), ccnt1, ccnt2,
                          rowScans.data(), rcnt1, rcnt2,
                          p1.data(), p2.data(), &cnt);
    std::vector<pos_t> fp1(cnt);
    std::vector<pos_t> fp2(cnt);
    uint16_t filteredCount1 = 0;
    uint16_t filteredCount2 = 0;
    weedOutPoints(p1.data(), cnt, fp1.data(), &filteredCount1);
    weedOutPoints(p2.data(), cnt, fp2.data(), &filteredCount2);
    float angle = 0;
    try {
        angle = calcAngle(fp1.data(), fp2.data(), std::min(filteredCount1, filteredCount2), width, height);
    } catch (const std::exception &ex) {
        // std::cerr << ex.what() << std::endl;
    };
    return angle;
}
#endif //TEST_SCAN_COLS_H