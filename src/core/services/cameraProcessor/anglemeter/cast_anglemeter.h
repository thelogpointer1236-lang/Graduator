#ifndef CAST_ANGLEMETER_H
#define CAST_ANGLEMETER_H

#include <vector>

// ------------------------------------
// Простейшие структуры координат
// ------------------------------------
struct pos_t {
    uint16_t x;
    uint16_t y;
};

// ------------------------------------
// Координата в виде float для точной геометрии
// ------------------------------------
struct posf_t {
    float x;
    float y;
};

// ------------------------------------
// Результаты поиска перепадов яркости по линии сканирования
// ------------------------------------
struct scan_t {
    pos_t posDifMin;
    pos_t posDifMax;
};

// ------------------------------------
// Основное состояние алгоритма измерения угла
// ------------------------------------
struct anglemeter_t {
    int img_width{};
    int img_height{};

    int bright_lim{};
    int max_pairs{};

    int scan_step;

    std::vector<scan_t> x_scans{};
    std::vector<scan_t> y_scans{};

    std::vector<posf_t> points_1{};
    std::vector<posf_t> points_2{};

    float last_angle_deg{};
    float (*transform_angle)(float) = nullptr;
};


void anglemeterCreate(anglemeter_t** am_ptr);
void anglemeterDestroy(anglemeter_t* am);
void anglemeterSetImageSize(anglemeter_t* am, int width, int height);
void anglemeterSetAngleTransformation(anglemeter_t* am, float (*func_ptr)(float));
void anglemeterRestoreState(anglemeter_t* am);

void anglemeterCreateImageBuffer(int width, int height);
bool anglemeterIsImageBufferCreated();
void anglemeterDestroyImageBuffer();
unsigned char* anglemeterCopyImage(const unsigned char* img);
void anglemeterFreeImage(const unsigned char* img);

// ----------------------------------------------------------------------------------------
// Основная функция вычисления угла стрелки
// На основании геометрии двух линий стрелки на изображении вычисляет значение угла,
// а также дополнительные значения (angle1, angle2), представляющие углы
// каждого из двух обнаруженных контуров отдельно.
// ----------------------------------------------------------------------------------------
bool anglemeterGetArrowAngle(anglemeter_t* am, const unsigned char *img, float* angle);

#endif //CAST_ANGLEMETER_H