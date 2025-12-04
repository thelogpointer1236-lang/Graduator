#include "UserDialogService.h"

#include <QThread>

namespace {
    const QString kPendingResponse = QStringLiteral("__PENDING_RESPONSE__");
}

UserDialogService::UserDialogService(QObject *parent) : QObject(parent) {}

QString UserDialogService::requestUserInput(const QString &title, const QString &message, const QStringList &options) {
    QString response = kPendingResponse;
    emit dialogRequested(title, message, options, &response);

    while (response == kPendingResponse) {
        QThread::msleep(50);
    }

    return response;
}
