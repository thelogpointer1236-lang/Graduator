#ifndef GRADUATOR_ASPECTRATIOLABEL_H
#define GRADUATOR_ASPECTRATIOLABEL_H
#include <QLabel>
class AspectRatioLabel final : public QLabel {
    Q_OBJECT
public:
    explicit AspectRatioLabel(const QString &text = "", QWidget *parent = nullptr);
    void setAspectRatio(double ratio);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
private:
    double aspectRatio;
};
#endif //GRADUATOR_ASPECTRATIOLABEL_H