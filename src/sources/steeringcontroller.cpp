#include "includes/steeringcontroller.hpp"
#include <QDebug>
#include <QtMath>

SteeringController::SteeringController(QObject *parent)
    : QObject(parent)
    , m_joystick(nullptr)
    , m_pollTimer(new QTimer(this))
    , m_steering(0.0)
    , m_throttle(0.0)
    , m_connected(false)
    , m_steeringAxis(0)
    , m_throttleAxis(2)
{
    initSDL();

    // Setup polling timer (60Hz = ~16ms)
    m_pollTimer->setInterval(16);
    connect(m_pollTimer, &QTimer::timeout, this, &SteeringController::pollJoystick);

    updateDeviceList();
}

SteeringController::~SteeringController()
{
    cleanupSDL();
}

void SteeringController::initSDL()
{
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
        qWarning() << "Failed to initialize SDL joystick:" << SDL_GetError();
        return;
    }
    qDebug() << "SDL Joystick initialized";
}

void SteeringController::cleanupSDL()
{
    closeJoystick();
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

void SteeringController::updateDeviceList()
{
    m_availableDevices.clear();
    m_deviceIndices.clear();

    SDL_JoystickUpdate();
    int numJoysticks = SDL_NumJoysticks();

    for (int i = 0; i < numJoysticks; ++i) {
        const char* name = SDL_JoystickNameForIndex(i);
        if (name) {
            m_availableDevices.append(QString::fromUtf8(name));
            m_deviceIndices.push_back(i);
        }
    }

    emit availableDevicesChanged();
    qDebug() << "Found" << numJoysticks << "joystick devices";
}

void SteeringController::refreshDevices()
{
    updateDeviceList();
}

void SteeringController::connectDevice(int index)
{
    if (index < 0 || index >= static_cast<int>(m_deviceIndices.size())) {
        qWarning() << "Invalid device index:" << index;
        return;
    }

    closeJoystick();
    openJoystick(m_deviceIndices[index]);
}

void SteeringController::disconnectDevice()
{
    closeJoystick();
}

void SteeringController::openJoystick(int sdlIndex)
{
    m_joystick = SDL_JoystickOpen(sdlIndex);
    if (!m_joystick) {
        qWarning() << "Failed to open joystick:" << SDL_GetError();
        m_connected = false;
        emit connectedChanged();
        return;
    }

    m_deviceName = QString::fromUtf8(SDL_JoystickName(m_joystick));
    m_connected = true;

    qDebug() << "Connected to:" << m_deviceName;
    qDebug() << "Axes:" << SDL_JoystickNumAxes(m_joystick);
    qDebug() << "Buttons:" << SDL_JoystickNumButtons(m_joystick);

    emit connectedChanged();
    emit deviceNameChanged();

    m_pollTimer->start();
}

void SteeringController::closeJoystick()
{
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }

    if (m_joystick) {
        SDL_JoystickClose(m_joystick);
        m_joystick = nullptr;
    }

    m_connected = false;
    m_deviceName.clear();
    m_steering = 0.0;
    m_throttle = 0.0;

    emit connectedChanged();
    emit deviceNameChanged();
    emit steeringChanged();
    emit throttleChanged();
}

void SteeringController::pollJoystick()
{
    if (!m_joystick) return;

    SDL_JoystickUpdate();

    int numAxes = SDL_JoystickNumAxes(m_joystick);

    // Steering (axis 0, normalized -1.0 to 1.0)
    if (m_steeringAxis < numAxes) {
        int rawSteering = SDL_JoystickGetAxis(m_joystick, m_steeringAxis);
        qreal newSteering = normalizeAxis(rawSteering);
        if (qAbs(newSteering - m_steering) > 0.001) {
            m_steering = newSteering;
            emit steeringChanged();
        }
    }

    // Throttle (axis 2, normalized 0.0 to 1.0, inverted)
    if (m_throttleAxis < numAxes) {
        int rawThrottle = SDL_JoystickGetAxis(m_joystick, m_throttleAxis);
        // Convert from -1.0..1.0 to 0.0..1.0 (inverted: -1.0 = full throttle)
        qreal newThrottle = (1.0 - normalizeAxis(rawThrottle)) / 2.0;
        if (qAbs(newThrottle - m_throttle) > 0.001) {
            m_throttle = newThrottle;
            emit throttleChanged();
        }
    }

}

qreal SteeringController::normalizeAxis(int value, int min, int max)
{
    // Normalize SDL axis value (-32768 to 32767) to -1.0 to 1.0
    if (value < min) value = min;
    if (value > max) value = max;

    qreal range = static_cast<qreal>(max - min);
    qreal normalized = (static_cast<qreal>(value - min) / range) * 2.0 - 1.0;

    return normalized;
}
