#ifndef STEERINGCONTROLLER_H
#define STEERINGCONTROLLER_H

#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <SDL2/SDL.h>
#include <vector>

class SteeringController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal steering READ steering NOTIFY steeringChanged)
    Q_PROPERTY(qreal throttle READ throttle NOTIFY throttleChanged)
    Q_PROPERTY(qreal brake READ brake NOTIFY brakeChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(QStringList availableDevices READ availableDevices NOTIFY availableDevicesChanged)
    
private:
    SDL_Joystick *m_joystick;
    QTimer *m_pollTimer;

    qreal m_steering;
    qreal m_throttle;
    qreal m_brake;
    bool m_connected;
    QString m_deviceName;
    QStringList m_availableDevices;
    std::vector<int> m_deviceIndices;

    int m_steeringAxis;
    int m_throttle;
    int m_brakeAxis;

public:
    explicit SteeringController(QObject *parent = nullptr);
    ~SteeringController();
};

#endif // STEERINGCONTROLLER_H