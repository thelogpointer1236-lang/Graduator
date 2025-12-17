#ifndef GRADUATOR_USERDIALOGSERVICE_H
#define GRADUATOR_USERDIALOGSERVICE_H

#include <QObject>

class UserDialogService final : public QObject {
    Q_OBJECT

signals:
    void dialogRequested(const QString &title,
                         const QString &message,
                         const QStringList &options,
                         QString *response);

    void infoRequested(const QString &title,
                       const QString &message,
                       bool *response);

    void warningRequested(const QString &title,
                          const QString &message,
                          bool *response);

    void errorRequested(const QString &title,
                        const QString &message,
                        bool *response);

public:
    explicit UserDialogService(QObject *parent = nullptr);

    QString requestUserInput(const QString &title,
                             const QString &message,
                             const QStringList &options);

    bool info(const QString &title, const QString &message);
    bool warning(const QString &title, const QString &message);
    bool error(const QString &title, const QString &message);
};

#endif // GRADUATOR_USERDIALOGSERVICE_H
