#include "UserDialogService.h"

#include <QThread>

namespace {
    constexpr bool kPendingBool = false;
    static QString kPendingString = QStringLiteral("_");
}

UserDialogService::UserDialogService(QObject *parent)
    : QObject(parent) {}

QString UserDialogService::requestUserInput(const QString &title,
                                           const QString &message,
                                           const QStringList &options)
{
    QString response = kPendingString;
    emit dialogRequested(title, message, options, &response);

    while (response == kPendingString) {
        QThread::msleep(50);
    }

    return response;
}

bool UserDialogService::info(const QString &title, const QString &message)
{
    bool response = false;
    emit infoRequested(title, message, &response);

    while (!response) {
        QThread::msleep(50);
    }

    return response;
}

bool UserDialogService::warning(const QString &title, const QString &message)
{
    bool response = false;
    emit warningRequested(title, message, &response);

    while (!response) {
        QThread::msleep(50);
    }

    return response;
}

bool UserDialogService::error(const QString &title, const QString &message)
{
    bool response = false;
    emit errorRequested(title, message, &response);

    while (!response) {
        QThread::msleep(50);
    }

    return response;
}
