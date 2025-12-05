#ifndef CAST_ANGLEMETER_H
#define CAST_ANGLEMETER_H


struct anglemeter_t;
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