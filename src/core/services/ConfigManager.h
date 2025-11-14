#ifndef GRADUATOR_CONFIGMANAGER_H
#define GRADUATOR_CONFIGMANAGER_H
#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QFile>
#include <QString>
#include <QVariant>
#include <QJsonArray>
template<typename T>
struct is_qvector : std::false_type {
};
template<typename U>
struct is_qvector<QVector<U> > : std::true_type {
};
template<typename U>
constexpr bool is_qvector_v = is_qvector<U>::value;
class ConfigManager final : public QObject {
    Q_OBJECT
public:
    explicit ConfigManager(const QString &path, QObject *parent = nullptr);
    bool load();
    bool save() const;
    QJsonValue getValue(const QString &keyPath, const QJsonValue &defaultValue = {}) const;
    void setValue(const QString &keyPath, const QJsonValue &value);
    QString configPath() const;
    bool isLoaded() const;
    template<typename T>
    T get(const QString &keyPath, const T &defaultValue = {}) const {
        QJsonValue v = getValue(keyPath);
        if (v.isUndefined() || v.isNull()) {
            // создаём запись в конфиге
            const_cast<ConfigManager *>(this)->setValue(
                keyPath, QJsonValue::fromVariant(QVariant::fromValue(defaultValue)));
            return defaultValue;
        }
        if constexpr (std::is_same_v<T, int>)
            return v.isDouble() ? v.toInt() : v.toString().toInt(nullptr, 0);
        if constexpr (std::is_same_v<T, double>)
            return v.isDouble() ? v.toDouble() : v.toString().toDouble();
        if constexpr (std::is_same_v<T, bool>)
            return v.isBool() ? v.toBool() : (v.toString().toLower() == "true");
        if constexpr (std::is_same_v<T, QString>)
            return v.toString();
        if constexpr (std::is_same_v<T, quint16>)
            return v.isDouble()
                       ? static_cast<quint16>(v.toInt())
                       : static_cast<quint16>(v.toString().toUShort(nullptr, 0));
        if constexpr (std::is_same_v<T, qint64>)
            return v.isDouble()
                       ? static_cast<qint64>(v.toDouble())
                       : v.toString().toLongLong(nullptr, 0);
        if constexpr (is_qvector_v<T>) {
            if (!v.isArray()) return defaultValue;
            QVector<int> result;
            for (const auto &item: v.toArray())
                result.append(item.toInt());
            return result;
        }
        // Fallback
        return defaultValue;
    }
    signals:
    
    void valueChanged(const QString &keyPath, const QJsonValue &value);
private:
    QJsonObject root;
    QString m_path;
    bool m_loaded = false;
    static QJsonValue getNestedValue(const QJsonObject &obj, const QStringList &path, const QJsonValue &def);
    static void setNestedValue(QJsonObject &obj, const QStringList &path, const QJsonValue &val);
};
/*  Пример использвоания:
    ConfigManager cfg("config.json");
    cfg.load();

    double kp = cfg.getValue("pressureController.pid.kp").toDouble();
    cfg.setValue("camera.exposure", 15.0);
    cfg.save();
 */
#endif //GRADUATOR_CONFIGMANAGER_H