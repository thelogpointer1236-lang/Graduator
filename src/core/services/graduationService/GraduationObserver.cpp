#include "GraduationObserver.h"
#include "core/services/ServiceLocator.h"
#include "core/utils/Derivator.h"
#include <QThread>
#include <QElapsedTimer>

#include "core/utils/Mediator.h"

GraduationObserver::GraduationObserver(QObject *parent)
    : QObject(parent)
{}

GraduationObserver::~GraduationObserver() {

}

bool GraduationObserver::isRunning() const {
    return m_isRunning.load();
}

void GraduationObserver::stop() {
    m_aboutToStop = true;
    while (m_isRunning) {
        QThread::msleep(10);
    }
    m_aboutToStop = false;
}

void GraduationObserver::start() {
    m_isRunning = true;

    auto* gs = ServiceLocator::instance().graduationService();
    auto* pc = ServiceLocator::instance().pressureController();
    auto* ps = ServiceLocator::instance().pressureSensor();
    auto* cp = ServiceLocator::instance().cameraProcessor();

    QElapsedTimer timer;
    Derivator Der_C_imp;
    Derivator Der_P;
    Derivator Der_A;

    Mediator Med_k_dC_imp;

    qreal k_dC_imp = 0.0;
    int bad_k_dC_imp = 0;

    while (true) {
        if (m_aboutToStop || QThread::currentThread()->isInterruptionRequested()) {
            break;
        }

        const bool isForwardDir = pc->g540Driver()->direction() == G540Direction::Forward;

        qreal t = timer.elapsed();
        Der_C_imp.push(t, pc->getImpulsesCount());
        Der_A.push()
        k_dC_imp = Der_C_imp.d(8) / qMax<qreal>(qAbs(Der_A.d(8)), 0.0001);

        if (pc->getImpulsesFreq() == 0) {
            Med_k_dC_imp.reset();
        }

        if (isForwardDir && Med_k_dC_imp.count() > 50) {
            if (k_dC_imp / Med_k_dC_imp.mean() > 5) {
                bad_k_dC_imp += 1;
            }
        }
        else {
            Med_k_dC_imp.push(k_dC_imp);
            bad_k_dC_imp -= bad_k_dC_imp > 0;
        }

        if (bad_k_dC_imp >= 10) {
            // Срочная остановка
            pc->g540Driver()->stop();
            pc->interrupt();
            emit restricted("Impulses were applied to the stepper motor, but for unknown reasons, the pressure did not change. For safety reasons, an emergency stop was performed. Please recheck the stand.");
        }

        QThread::msleep(100);
    }
    m_isRunning = false;
}
