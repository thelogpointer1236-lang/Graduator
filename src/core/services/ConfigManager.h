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
#include <QColor>

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

    template<typename U>
    U convertJsonValue(const QJsonValue &v) {
        if constexpr (std::is_same_v<U, int>) {
            return v.isDouble() ? v.toInt() : v.toString().toInt(nullptr, 0);
        }
        if constexpr (std::is_same_v<U, double>) {
            return v.isDouble() ? v.toDouble() : v.toString().toDouble();
        }
        if constexpr (std::is_same_v<U, bool>) {
            return v.isBool() ? v.toBool() : (v.toString().toLower() == "true");
        }
        if constexpr (std::is_same_v<U, QString>) {
            return v.toString();
        }
        if constexpr (std::is_same_v<U, quint16>) {
            return v.isDouble()
                       ? static_cast<quint16>(v.toInt())
                       : static_cast<quint16>(v.toString().toUShort(nullptr, 0));
        }
        if constexpr (std::is_same_v<U, qint64>) {
            return v.isDouble()
                       ? static_cast<qint64>(v.toDouble())
                       : v.toString().toLongLong(nullptr, 0);
        }
        if constexpr (std::is_same_v<U, QColor>) {
            // Строка: "#RRGGBB" или имя цвета
            if (v.isString()) {
                QColor c(v.toString());
                return c.isValid() ? c : QColor();
            }

            // Число: 0xRRGGBB или 0xAARRGGBB
            if (v.isDouble()) {
                uint val = v.toInt();
                if (val <= 0xFFFFFF)
                    return QColor::fromRgb(val);
                else
                    return QColor::fromRgba(val);
            }

            // JSON-объект: {r,g,b,a?}
            if (v.isObject()) {
                QJsonObject o = v.toObject();
                int r = o.value("r").toInt(0);
                int g = o.value("g").toInt(0);
                int b = o.value("b").toInt(0);
                int a = o.value("a").toInt(255);
                return QColor(r, g, b, a);
            }

            return QColor();
        }

        // fallback: если тип неподдерживаем
        return U{};
    }

    template<typename T>
    T get(const QString &keyPath, const T &defaultValue = {}) {
        QJsonValue v = getValue(keyPath);
        if (v.isUndefined() || v.isNull()) {
            this->setValue(
                keyPath, QJsonValue::fromVariant(QVariant::fromValue(defaultValue)));
            return defaultValue;
        }

        // Обычные типы
        if constexpr (!is_qvector_v<T>) {
            return convertJsonValue<T>(v);
        }

        // Универсальный QVector<U>
        if constexpr (is_qvector_v<T>) {
            using U = typename T::value_type;
            if (!v.isArray())
                return defaultValue;

            T result;
            result.reserve(v.toArray().size());
            for (const auto &item: v.toArray())
                result.append(convertJsonValue<U>(item));
            return result;
        }

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

#endif //GRADUATOR_CONFIGMANAGER_H