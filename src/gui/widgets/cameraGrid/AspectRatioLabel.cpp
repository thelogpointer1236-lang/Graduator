#include "AspectRatioLabel.h"
AspectRatioLabel::AspectRatioLabel(const QString &text, QWidget *parent) : QLabel(text, parent),
                                                                           aspectRatio(4.0 / 3.0) {
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setAlignment(Qt::AlignCenter);
}
void AspectRatioLabel::setAspectRatio(double ratio) {
    aspectRatio = ratio;
    updateGeometry();
}
QSize AspectRatioLabel::sizeHint() const {
    // Базовый размер, сохраняющий соотношение сторон
    int w = 320; // стандартная ширина
    return QSize(w, static_cast<int>(w / aspectRatio));
}
QSize AspectRatioLabel::minimumSizeHint() const {
    return QSize(80, 60); // Минимальный разумный размер
}