#ifndef GRADUATOR_USERDIALOGSERVICE_H
#define GRADUATOR_USERDIALOGSERVICE_H

#include <QObject>

class UserDialogService final : public QObject {
    Q_OBJECT
signals:
    void dialogRequested(const QString &title, const QString &message, const QStringList &options, QString *response);

public:
    explicit UserDialogService(QObject *parent = nullptr);

    QString requestUserInput(const QString &title, const QString &message, const QStringList &options);
};

#endif //GRADUATOR_USERDIALOGSERVICE_H
