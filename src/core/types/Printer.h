#ifndef GRADUATOR_PRINTER_H
#define GRADUATOR_PRINTER_H
#include <QString>

class Printer {
public:
    Printer() = default;
    Printer(const QString& name, const QString& folder)
        : name_(name), folder_(folder) {}
    QString name() const { return name_; }
    QString folder() const { return folder_; }
private:
    QString name_;
    QString folder_;
};

#endif //GRADUATOR_PRINTER_H