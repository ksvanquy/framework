#pragma once

#include <memory>

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include "runtime/runtime.h"

namespace framework::ui {

class RuntimeBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool moduleRegistered READ moduleRegistered NOTIFY moduleRegisteredChanged)

public:
    explicit RuntimeBridge(QObject* parent = nullptr);
    ~RuntimeBridge() override;

    [[nodiscard]] QString state() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] bool moduleRegistered() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void clearError();

signals:
    void stateChanged();
    void moduleRegisteredChanged();
    void lastErrorChanged();
    void errorOccurred(const QString& message);

private:
    void setError(QString message);
    void setState(QString state);

    std::unique_ptr<runtime::Runtime> runtime_;
    bool moduleRegistered_ = false;
    QString state_ = QStringLiteral("Stopped");
    QString lastError_;
};

} // namespace framework::ui