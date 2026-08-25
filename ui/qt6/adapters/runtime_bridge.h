#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include "runtime/runtime.h"

namespace framework::ui {

class RuntimeBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)

public:
    explicit RuntimeBridge(QObject* parent = nullptr);
    ~RuntimeBridge() override;

    [[nodiscard]] QString state() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void stateChanged();
    void errorOccurred(const QString& message);

private:
    void setState(QString state);

    runtime::Runtime runtime_;
    bool moduleRegistered_ = false;
    QString state_ = QStringLiteral("Stopped");
};

} // namespace framework::ui