#include "fpscounter.h"
#include <QtCore/qtimer.h>
#include <QtQuick/qquickwindow.h>

class FPSCounterPrivate final {
    Q_DISABLE_COPY(FPSCounterPrivate)
    Q_DECLARE_PUBLIC(FPSCounter)

public:
    FPSCounterPrivate(FPSCounter* qq);
    ~FPSCounterPrivate();

    FPSCounter* q_ptr{ nullptr };
    int frameCount{ 0 };
    QMetaObject::Connection connection{};
};

FPSCounterPrivate::FPSCounterPrivate(FPSCounter* qq) : q_ptr{ qq } {
    QObject::connect(q_ptr, &FPSCounter::windowChanged, q_ptr, [this](QQuickWindow* window){
        if (connection) {
            QObject::disconnect(std::exchange(connection, {}));
        }
        if (window) {
            connection = QObject::connect(window, &QQuickWindow::frameSwapped, q_ptr, [this](){ ++frameCount; });
        }
    });
    auto timer = new QTimer(q_ptr);
    timer->setTimerType(Qt::PreciseTimer);
    QObject::connect(timer, &QTimer::timeout, q_ptr, [this](){
        Q_EMIT q_ptr->valueChanged();
        frameCount = 0;
    });
    timer->start(1000);
}

FPSCounterPrivate::~FPSCounterPrivate() = default;

FPSCounter::FPSCounter(QQuickItem* parent) : QQuickItem{ parent }, d_ptr{ std::make_unique<FPSCounterPrivate>(this) } {}

FPSCounter::~FPSCounter() = default;

int FPSCounter::value() const {
    Q_D(const FPSCounter);
    return d->frameCount;
}
