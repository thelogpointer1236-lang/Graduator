#include "RGBImage.h"
#include <map>
#include <stdexcept>
#include <string>
RGBPixel::RGBPixel() : r(0), g(0), b(0) {
}
RGBPixel::RGBPixel(unsigned char red, unsigned char green, unsigned char blue)
    : r(red), g(green), b(blue) {
}
RGBImage::RGBImage(unsigned char *buffer, const long bufferSizeInBytes) : m_pixelData(buffer) {
    static const std::map<long, std::pair<int, int> > bufferToSizeMap = {
        {230400, {320, 240}}, // 320×240 × 3 = 230,400 байт
        {307200, {320, 320}}, // 320×240 × 3 = 230,400 байт
        {921600, {640, 480}}, // 640×480 × 3 = 921,600 байт
        {2073600, {1280, 720}}, // 1280×720 × 3 = 2,073,600 байт
        {8294400, {1920, 1080}}, // 1920×1080 × 3 = 8,294,400 байт
        {3110400, {1440, 1080}}, // 1440×1080 × 3 = 3,110,400 байт
        {3145728, {1280, 816}}, // 1280×816 × 3 = 3,145,728 байт
        {153600, {320, 160}}, // 320×160 × 3 = 153,600 байт (часто для веб-камер)
        {460800, {640, 240}} // 640×240 × 3 = 460,800 байт (широкоформатная веб-камера)
    };
    const auto size = bufferToSizeMap.find(bufferSizeInBytes);
    if (size == bufferToSizeMap.end()) {
        throw std::invalid_argument("Invalid buffer size: " + std::to_string(bufferSizeInBytes));
    }
    this->m_width = size->second.first;
    this->m_height = size->second.second;
}
int RGBImage::getWidth() const { return m_width; }
int RGBImage::getHeight() const { return m_height; }
RGBPixel RGBImage::getPixel(int x, int y) const {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        // Учитываем обратный порядок строк
        int reversedY = m_height - 1 - y;
        int pixelIndex = (reversedY * m_width + x) * 3;
        return RGBPixel(m_pixelData[pixelIndex + 2], // r
                        m_pixelData[pixelIndex + 1], // g
                        m_pixelData[pixelIndex]); // b
    }
    return RGBPixel(0, 0, 0); // Возвращаем черный пиксель при выходе за границы
}
void RGBImage::setPixel(int x, int y, const RGBPixel &pixel) {
    setPixel(x, y, pixel.r, pixel.g, pixel.b);
}
void RGBImage::setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        // Учитываем обратный порядок строк
        int reversedY = m_height - 1 - y;
        int pixelIndex = (reversedY * m_width + x) * 3;
        m_pixelData[pixelIndex + 2] = r;
        m_pixelData[pixelIndex + 1] = g;
        m_pixelData[pixelIndex] = b;
    }
}
void RGBImage::drawCircle(int centerX, int centerY, int radius,
                          const RGBPixel &color, int thickness) {
    // Рисуем несколько концентрических окружностей для создания толщины
    for (int i = 0; i < thickness; i++) {
        int currentRadius = radius + i;
        // Рисуем круг с помощью алгоритма Брезенхема для окружностей
        int x = 0;
        int y = currentRadius;
        int d = 3 - 2 * currentRadius;
        while (x <= y) {
            // Рисуем 8 симметричных точек
            setPixel(centerX + x, centerY + y, color.r, color.g, color.b);
            setPixel(centerX - x, centerY + y, color.r, color.g, color.b);
            setPixel(centerX + x, centerY - y, color.r, color.g, color.b);
            setPixel(centerX - x, centerY - y, color.r, color.g, color.b);
            setPixel(centerX + y, centerY + x, color.r, color.g, color.b);
            setPixel(centerX - y, centerY + x, color.r, color.g, color.b);
            setPixel(centerX + y, centerY - x, color.r, color.g, color.b);
            setPixel(centerX - y, centerY - x, color.r, color.g, color.b);
            if (d < 0) {
                d = d + 4 * x + 6;
            } else {
                d = d + 4 * (x - y) + 10;
                y--;
            }
            x++;
        }
    }
}
void RGBImage::drawDigit(int x, int y, int digit, const RGBPixel &color, int scale) {
    // Шаблоны для цифр 0-9 (7x5 пикселей)
    static const bool digitPatterns[10][7][5] = {
        // 0
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 1
        {
            {0, 0, 1, 0, 0},
            {0, 1, 1, 0, 0},
            {1, 0, 1, 0, 0},
            {0, 0, 1, 0, 0},
            {0, 0, 1, 0, 0},
            {0, 0, 1, 0, 0},
            {1, 1, 1, 1, 1}
        },
        // 2
        {
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 0},
            {1, 1, 1, 1, 1}
        },
        // 3
        {
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 4
        {
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {0, 0, 0, 0, 1},
            {0, 0, 0, 0, 1}
        },
        // 5
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 0},
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 6
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 0},
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 7
        {
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {0, 0, 0, 1, 0},
            {0, 0, 1, 0, 0},
            {0, 1, 0, 0, 0},
            {1, 0, 0, 0, 0},
            {1, 0, 0, 0, 0}
        },
        // 8
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        },
        // 9
        {
            {1, 1, 1, 1, 1},
            {1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1},
            {1, 1, 1, 1, 1},
            {0, 0, 0, 0, 1},
            {0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1}
        }
    };
    // Точка
    static const bool dotPattern[3][1] = {
        {0},
        {0},
        {1}
    };
    if (digit >= 0 && digit <= 9) {
        for (int py = 0; py < 7; py++) {
            for (int px = 0; px < 5; px++) {
                if (digitPatterns[digit][py][px]) {
                    for (int sy = 0; sy < scale; sy++) {
                        for (int sx = 0; sx < scale; sx++) {
                            setPixel(x + px * scale + sx, y + py * scale + sy, color);
                        }
                    }
                }
            }
        }
    } else if (digit == -1) {
        // Точка
        for (int py = 0; py < 3; py++) {
            for (int px = 0; px < 1; px++) {
                if (dotPattern[py][px]) {
                    for (int sy = 0; sy < scale; sy++) {
                        for (int sx = 0; sx < scale; sx++) {
                            setPixel(x + px * scale + sx, y + py * scale + sy, color);
                        }
                    }
                }
            }
        }
    }
}
void RGBImage::drawFloatNumber(int x, int y, float number, int decimalPlaces, const RGBPixel &color, int scale) {
    // Определяем знак
    bool isNegative = number < 0;
    if (isNegative) {
        number = -number;
    }
    // Округляем до нужного количества знаков после запятой
    float multiplier = 1.0f;
    for (int i = 0; i < decimalPlaces; i++) {
        multiplier *= 10.0f;
    }
    int scaledNumber = (int) (number * multiplier + 0.5f);
    // Извлекаем цифры после запятой
    int digitsAfterPoint[10]; // Максимум 10 знаков
    int afterPointCount = 0;
    for (int i = 0; i < decimalPlaces; i++) {
        digitsAfterPoint[decimalPlaces - 1 - i] = scaledNumber % 10;
        scaledNumber /= 10;
        afterPointCount++;
    }
    // Извлекаем цифры до запятой
    int digitsBeforePoint[10]; // Максимум 10 знаков
    int beforePointCount = 0;
    if (scaledNumber == 0) {
        digitsBeforePoint[0] = 0;
        beforePointCount = 1;
    } else {
        while (scaledNumber > 0) {
            digitsBeforePoint[beforePointCount] = scaledNumber % 10;
            scaledNumber /= 10;
            beforePointCount++;
        }
    }
    // Рисуем знак минуса если нужно
    int currentX = x;
    if (isNegative) {
        // Рисуем минус (простая линия)
        for (int i = 0; i < 5 * scale; i++) {
            for (int j = 0; j < scale; j++) {
                setPixel(currentX + i, y + 3 * scale + j, color);
            }
        }
        currentX += 6 * scale; // Сдвигаем позицию
    }
    // Рисуем цифры до запятой (в правильном порядке)
    for (int i = beforePointCount - 1; i >= 0; i--) {
        drawDigit(currentX, y, digitsBeforePoint[i], color, scale);
        currentX += 6 * scale; // Ширина цифры + пробел
    }
    // Рисуем точку
    drawDigit(currentX, y + 5 * scale, -1, color, scale); // Точка
    currentX += 2 * scale; // Ширина точки + пробел
    // Рисуем цифры после запятой
    for (int i = 0; i < afterPointCount; i++) {
        drawDigit(currentX, y, digitsAfterPoint[i], color, scale);
        currentX += 6 * scale; // Ширина цифры + пробел
    }
}