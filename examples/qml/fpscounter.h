#pragma once

#include <QtQuick/qquickitem.h>
#include <memory>

class FPSCounterPrivate;
class FPSCounter : public QQuickItem {
    Q_OBJECT
    Q_DECLARE_PRIVATE(FPSCounter)
    Q_PROPERTY(int value READ value NOTIFY valueChanged FINAL)
#ifdef QML_ELEMENT
    QML_ELEMENT
#endif

public:
    explicit FPSCounter(QQuickItem* parent = nullptr);
    ~FPSCounter() override;

    int value() const;

Q_SIGNALS:
    void valueChanged();

private:
    const std::unique_ptr<FPSCounterPrivate> d_ptr;
};
