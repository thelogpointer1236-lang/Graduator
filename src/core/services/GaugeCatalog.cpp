#include "GaugeCatalog.h"
#include "ServiceLocator.h"
#include <QDir>
#include <QSet>
GaugeCatalog::GaugeCatalog(QObject *parent)
    : QObject(parent) {
}
bool GaugeCatalog::loadFromDirectory(const QString &dirPath) {
    auto *log = ServiceLocator::instance().logger();
    QDir dir(dirPath);
    if (!dir.exists()) {
        if (log) log->error(QString::fromWCharArray(L"Каталог не найден: %1").arg(dirPath));
        return false;
    }
    const QStringList files = dir.entryList(QStringList() << "*.cfm", QDir::Files);
    if (files.isEmpty()) {
        if (log) log->warn(QString::fromWCharArray(L"В каталоге %1 нет файлов моделей").arg(dirPath));
        return false;
    }
    QList<GaugeModel> temp;
    for (const QString &fileName: files) {
        const QString fullPath = dir.absoluteFilePath(fileName);
        GaugeModel model;
        if (loadSingleFile(fullPath, model)) {
            temp.append(model);
            if (log) log->info(QString::fromWCharArray(L"Загружена модель %1").arg(model.name()));
        } else {
            if (log) log->warn(QString::fromWCharArray(L"Пропуск файла: %1 (ошибка разбора)").arg(fullPath));
        }
    }
    if (temp.isEmpty()) {
        if (log) log->error(L"Не удалось загрузить ни одной модели манометра");
        return false;
    }
    m_models = std::move(temp);
    if (log) log->info(QString::fromWCharArray(L"Загружено %1 моделей манометров").arg(m_models.size()));
    return true;
}
bool GaugeCatalog::loadSingleFile(const QString &filePath, GaugeModel &outModel) {
    QFile file(filePath);
    auto *log = ServiceLocator::instance().logger();
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        if (log)
            log->warn(QString::fromWCharArray(L"Не удалось открыть файл %1: %2")
                .arg(filePath, file.errorString()));
        return false;
    }
    QTextStream in(&file);
    const QStringList lines = in.readAll().split('\n', QString::SkipEmptyParts);
    if (lines.size() < 6) {
        if (log)
            log->warn(QString::fromWCharArray(L"Файл %1 некорректен: слишком мало строк (%2 < 6)")
                .arg(filePath).arg(lines.size()));
        return false;
    }
    GaugeModel model;
    model.setName(QFileInfo(filePath).baseName());
    model.setPrintingTemplate(lines[2].trimmed());
    const QStringList tokens = lines[5].split(' ', QString::SkipEmptyParts);
    std::vector<double> pressureValues;
    pressureValues.reserve(tokens.size());
    for (const QString &token: tokens) {
        bool ok = false;
        const double value = token.toDouble(&ok);
        if (!ok) {
            if (log)
                log->warn(QString::fromWCharArray(L"Ошибка парсинга числа в файле %1: '%2'")
                    .arg(filePath, token));
            return false;
        }
        pressureValues.emplace_back(value);
    }
    if (pressureValues.empty()) {
        if (log) log->warn(QString::fromWCharArray(L"Файл %1 не содержит точек давления").arg(filePath));
        return false;
    }
    model.setPressureValues(pressureValues);
    outModel = std::move(model);
    return true;
}
const QList<GaugeModel> &GaugeCatalog::all() const noexcept {
    return m_models;
}
const QStringList &GaugeCatalog::allNames() const noexcept {
    static QStringList names;
    if (names.size() != m_models.size()) {
        names.clear();
        for (const auto &m: m_models)
            names << m.name();
    }
    return names;
}
const QStringList &GaugeCatalog::allPrintingTemplates() const noexcept {
    static QStringList templates;
    if (templates.size() != m_models.size()) {
        QSet<QString> unique;
        templates.clear();
        for (const auto &m: m_models)
            unique.insert(m.printingTemplate());
        for (const auto &m: unique)
            templates.append(m);
        std::sort(templates.begin(), templates.end(), [](const QString &a, const QString &b) {
            return a.localeAwareCompare(b) < 0;
        });
    }
    return templates;
}
const QStringList &GaugeCatalog::allPressureUnits() const noexcept {
    static const QStringList units{
        QString::fromWCharArray(L"Па"),
        QString::fromWCharArray(L"кПа"),
        QString::fromWCharArray(L"МПа"),
        QString::fromWCharArray(L"Бар"),
        QString::fromWCharArray(L"кгс/см²"),
        QString::fromWCharArray(L"Psi")
    };
    return units;
}
const QStringList &GaugeCatalog::allPrinters() const noexcept {
    static const QStringList printers{
        QString::fromWCharArray(L"A3"),
        QString::fromWCharArray(L"A2 первый"),
        QString::fromWCharArray(L"A2 второй")
    };
    return printers;
}
const QStringList &GaugeCatalog::allPrecisions() const noexcept {
    static const QStringList precisions{
        "1,5", "2,5", "4"
    };
    return precisions;
}
const GaugeModel *GaugeCatalog::findByName(const QString &name) const noexcept {
    for (const auto &m: m_models)
        if (m.name().compare(name, Qt::CaseInsensitive) == 0)
            return &m;
    return nullptr;
}
const GaugeModel *GaugeCatalog::findByIdx(const int idx) const noexcept {
    if (idx < 0 || idx >= m_models.size())
        return nullptr;
    return &m_models[idx];
}