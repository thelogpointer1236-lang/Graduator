#ifndef BMPIMAGE24_H
#define BMPIMAGE24_H
#include <vector>
#include <cmath>
#include <algorithm>
struct RGBPixel {
    unsigned char b, g, r;
    RGBPixel();
    RGBPixel(unsigned char red, unsigned char green, unsigned char blue);
};
class RGBImage {
private:
    int m_width;
    int m_height;
    unsigned char *m_pixelData; // Указатель на буфер пикселей без копирования
public:
    // Конструктор - принимает буфер без копирования
    RGBImage(unsigned char *buffer, long bufferSizeInBytes);
    // Геттеры
    int getWidth() const;
    int getHeight() const;
    // Получение пикселя
    RGBPixel getPixel(int x, int y) const;
    // Установка пикселя
    void setPixel(int x, int y, const RGBPixel &pixel);
    // Установка пикселя по RGB значениям
    void setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
    // Рисование окружности (полный круг 0-360 градусов)
    void drawCircle(int centerX, int centerY, int radius,
                    const RGBPixel &color, int thickness);
private:
    // Вспомогательная функция для рисования одной цифры
    void drawDigit(int x, int y, int digit, const RGBPixel &color, int scale);
public:
    // Рисование числа с плавающей точкой
    void drawFloatNumber(int x, int y, float number, int decimalPlaces,
                         const RGBPixel &color, int scale = 1);
};
#endif // BMPIMAGE24_H