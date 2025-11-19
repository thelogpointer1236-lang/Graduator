#ifndef GRADUATOR_DISPLACEMENT_H
#define GRADUATOR_DISPLACEMENT_H
#include <QString>

class Displacement {
public:
    Displacement() = default;
    Displacement(const QString& name, int num)
        : name_(name), num_(num) {}
    QString name() const { return name_; }
    int num() const { return num_; }
private:
    QString name_;
    int num_;
};

#endif //GRADUATOR_DISPLACEMENT_H