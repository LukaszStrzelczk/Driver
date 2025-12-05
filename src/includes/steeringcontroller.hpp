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
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(QStringList availableDevices READ availableDevices NOTIFY availableDevicesChanged)

public:
    explicit SteeringController(QObject *parent = nullptr);
    ~SteeringController();

    // Property getters
    qreal steering() const { return m_steering; }
    qreal throttle() const { return m_throttle; }
    bool connected() const { return m_connected; }
    QString deviceName() const { return m_deviceName; }
    QStringList availableDevices() const { return m_availableDevices; }

    // Public methods
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void connectDevice(int index);
    Q_INVOKABLE void disconnectDevice();

signals:
    void steeringChanged();
    void throttleChanged();
    void connectedChanged();
    void deviceNameChanged();
    void availableDevicesChanged();

private slots:
    void pollJoystick();

private:
    // SDL members
    SDL_Joystick *m_joystick;
    QTimer *m_pollTimer;

    // Input values (normalized)
    qreal m_steering;
    qreal m_throttle;

    // Connection state
    bool m_connected;
    QString m_deviceName;
    QStringList m_availableDevices;
    std::vector<int> m_deviceIndices;

    // Axis mapping
    int m_steeringAxis;
    int m_throttleAxis;

    // Private methods
    void initSDL();
    void cleanupSDL();
    void updateDeviceList();
    void openJoystick(int index);
    void closeJoystick();
    qreal normalizeAxis(int value, int min = -32768, int max = 32767);
};

#endif // STEERINGCONTROLLER_H