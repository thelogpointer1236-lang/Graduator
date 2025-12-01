#ifndef GRADUATOR_GRADUATIONOBSERVER_H
#define GRADUATOR_GRADUATIONOBSERVER_H
#include <QObject>
#include <atomic>
#include <functional>



class GraduationObserver final : public QObject {
    Q_OBJECT
public:
    explicit GraduationObserver(QObject *parent = nullptr);
    ~GraduationObserver();

    bool isRunning() const;
    void stop();

public slots:
    void start();

signals:
    void restricted(QString message);

private:
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_aboutToStop{false};
};

#endif //GRADUATOR_GRADUATIONOBSERVER_H