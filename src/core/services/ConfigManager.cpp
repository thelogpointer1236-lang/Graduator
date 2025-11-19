#include "ConfigManager.h"

ConfigManager::ConfigManager(const QString &path, QObject *parent)
    : QObject(parent), m_path(path) {
}

bool ConfigManager::load() {
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    root = doc.object();
    m_loaded = true;
    return true;
}

bool ConfigManager::save() const {
    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QJsonDocument doc(root);
    f.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

QJsonValue ConfigManager::getValue(const QString &keyPath, const QJsonValue &defaultValue) const {
    return getNestedValue(root, keyPath.split('.'), defaultValue);
}

void ConfigManager::setValue(const QString &keyPath, const QJsonValue &value) {
    setNestedValue(root, keyPath.split('.'), value);
    emit valueChanged(keyPath, value);
}

QString ConfigManager::configPath() const { return m_path; }

bool ConfigManager::isLoaded() const { return m_loaded; }

QJsonValue ConfigManager::getNestedValue(const QJsonObject &obj, const QStringList &path, const QJsonValue &def) {
    QJsonValue current = obj;
    for (const QString &key: path) {
        if (!current.isObject()) return def;
        QJsonObject o = current.toObject();
        if (!o.contains(key)) return def;
        current = o.value(key);
    }
    return current;
}

void ConfigManager::setNestedValue(QJsonObject &obj, const QStringList &path, const QJsonValue &val) {
    if (path.isEmpty()) return;
    const QString &key = path.first();
    if (path.size() == 1) {
        obj[key] = val;
        return;
    }
    QJsonObject sub;
    if (obj.contains(key) && obj[key].isObject())
        sub = obj[key].toObject();
    // Рекурсивно создаём недостающие вложенные объекты
    setNestedValue(sub, path.mid(1), val);
    obj[key] = sub;
}
