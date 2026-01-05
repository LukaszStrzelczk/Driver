/**
 * @file steeringcontroller.cpp
 * @brief Implementation of steering wheel and game controller input handling
 *
 * This file implements the SteeringController class defined in steeringcontroller.hpp.
 * It uses SDL2's joystick API to detect, connect to, and read input from physical
 * steering wheels, pedals, and game controllers. Values are normalized and exposed
 * to QML for use in driving applications.
 */

#include "includes/steeringcontroller.hpp"
#include <QDebug>    // Qt logging for debugging output
#include <QtMath>    // Qt math utilities (qAbs for absolute value)

/**
 * @brief Constructor - initializes SDL2 and sets up polling timer
 * @param parent Parent QObject for memory management (follows Qt parent-child pattern)
 *
 * Initializes all member variables, sets up SDL2's joystick subsystem, configures
 * the polling timer for 60Hz updates (~16ms interval), and scans for available devices.
 *
 * Default axis mapping:
 * - Axis 0: Steering wheel (left/right)
 * - Axis 2: Throttle/brake pedal (combined or separate depending on device)
 */
SteeringController::SteeringController(QObject *parent)
    : QObject(parent)              // Initialize QObject base class with parent
    , m_joystick(nullptr)          // No joystick connected initially
    , m_pollTimer(new QTimer(this))  // Create polling timer (Qt parent system will delete it)
    , m_steering(0.0)              // Start with centered steering
    , m_throttle(0.0)              // Start with neutral throttle
    , m_connected(false)           // Not connected to any device initially
    , m_steeringAxis(0)            // Default to axis 0 for steering
    , m_throttleAxis(2)            // Default to axis 2 for throttle (common for pedals)
{
    // Initialize SDL2's joystick subsystem
    initSDL();

    // Setup polling timer to run at 60Hz for smooth input updates
    // 60Hz = 1000ms / 60 ≈ 16.67ms, rounded to 16ms
    m_pollTimer->setInterval(16);
    connect(m_pollTimer, &QTimer::timeout, this, &SteeringController::pollJoystick);

    // Scan for available devices and populate device list
    updateDeviceList();
}

/**
 * @brief Destructor - ensures proper cleanup of SDL2 resources
 *
 * Closes any open joystick connection and shuts down SDL2 subsystems.
 * Called automatically when the object is deleted (either explicitly or by Qt parent system).
 */
SteeringController::~SteeringController()
{
    cleanupSDL();
}

/**
 * @brief Initializes SDL2's joystick subsystem
 *
 * Called in constructor to set up SDL2. Only initializes the joystick subsystem,
 * not video, audio, or other SDL2 features. Logs error if initialization fails.
 *
 * Note: SDL_InitSubSystem is safe to call multiple times - it will only initialize once.
 */
void SteeringController::initSDL()
{
    // Initialize only the joystick subsystem (we don't need video, audio, etc.)
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
        qWarning() << "Failed to initialize SDL joystick:" << SDL_GetError();
        return;
    }
    qDebug() << "SDL Joystick initialized";
}

/**
 * @brief Cleans up and shuts down SDL2 subsystems
 *
 * Called in destructor to properly close any open joystick connections and
 * shut down SDL2's joystick subsystem. Ensures clean shutdown.
 */
void SteeringController::cleanupSDL()
{
    // Close any open joystick first
    closeJoystick();
    // Shut down SDL's joystick subsystem
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

/**
 * @brief Scans for connected devices and updates internal device lists
 *
 * Queries SDL2 for all connected joystick/game controller devices and populates:
 * - m_availableDevices: User-friendly device names for display in UI
 * - m_deviceIndices: Corresponding SDL device indices for connection
 *
 * These lists are kept in parallel (same index in both lists refers to same device).
 * Emits availableDevicesChanged signal to notify QML of updated device list.
 */
void SteeringController::updateDeviceList()
{
    // Clear existing device lists
    m_availableDevices.clear();
    m_deviceIndices.clear();

    // Update SDL's internal joystick state
    SDL_JoystickUpdate();

    // Query how many joystick devices are connected
    int numJoysticks = SDL_NumJoysticks();

    // Iterate through all detected devices
    for (int i = 0; i < numJoysticks; ++i) {
        // Get the device name from SDL
        const char* name = SDL_JoystickNameForIndex(i);
        if (name) {
            // Add to user-friendly list (converted from UTF-8 to QString)
            m_availableDevices.append(QString::fromUtf8(name));
            // Store the SDL device index (parallel array)
            m_deviceIndices.push_back(i);
        }
    }

    // Notify QML that the device list has changed
    emit availableDevicesChanged();
    qDebug() << "Found" << numJoysticks << "joystick devices";
}

/**
 * @brief Public method to refresh the list of available devices
 *
 * Called from QML when user wants to rescan for devices (e.g., after plugging
 * in a new controller). Simply delegates to updateDeviceList().
 */
void SteeringController::refreshDevices()
{
    updateDeviceList();
}

/**
 * @brief Connects to a specific input device by list index
 * @param index Index into the availableDevices list (0-based)
 *
 * Converts the list index to an SDL device index and attempts to open
 * that joystick device. If another device is already connected, it will
 * be disconnected first. Validates index bounds before attempting connection.
 */
void SteeringController::connectDevice(int index)
{
    // Validate that index is within bounds of device list
    if (index < 0 || index >= static_cast<int>(m_deviceIndices.size())) {
        qWarning() << "Invalid device index:" << index;
        return;
    }

    // Close any existing connection first
    closeJoystick();

    // Open the joystick using its SDL device index (not the list index)
    openJoystick(m_deviceIndices[index]);
}

/**
 * @brief Disconnects the currently connected input device
 *
 * Public method callable from QML to disconnect the active device.
 * Simply delegates to closeJoystick(). Safe to call even if no device is connected.
 */
void SteeringController::disconnectDevice()
{
    closeJoystick();
}

/**
 * @brief Opens a specific joystick device and starts polling
 * @param sdlIndex SDL device index (from SDL_NumJoysticks enumeration, not list position)
 *
 * Opens the specified SDL joystick device, retrieves its name, logs its capabilities
 * (number of axes and buttons), updates connection state, and starts the polling timer.
 *
 * If the joystick fails to open (device unplugged, permissions issue, etc.), updates
 * connection state to false and returns without starting polling.
 */
void SteeringController::openJoystick(int sdlIndex)
{
    // Attempt to open the joystick device
    m_joystick = SDL_JoystickOpen(sdlIndex);
    if (!m_joystick) {
        qWarning() << "Failed to open joystick:" << SDL_GetError();
        m_connected = false;
        emit connectedChanged();
        return;
    }

    // Get the device name from the opened joystick
    m_deviceName = QString::fromUtf8(SDL_JoystickName(m_joystick));
    m_connected = true;

    // Log connection success and device capabilities
    qDebug() << "Connected to:" << m_deviceName;
    qDebug() << "Axes:" << SDL_JoystickNumAxes(m_joystick);
    qDebug() << "Buttons:" << SDL_JoystickNumButtons(m_joystick);

    // Notify QML of connection state and device name changes
    emit connectedChanged();
    emit deviceNameChanged();

    // Start polling the joystick for input
    m_pollTimer->start();
}

/**
 * @brief Closes the currently open joystick device
 *
 * Stops the polling timer, closes the SDL joystick handle, resets all input values
 * to zero, and clears connection state. Safe to call even if no joystick is open.
 *
 * Emits signals for all changed properties (connected, device name, steering, throttle).
 */
void SteeringController::closeJoystick()
{
    // Stop polling if timer is running
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }

    // Close the SDL joystick handle if open
    if (m_joystick) {
        SDL_JoystickClose(m_joystick);
        m_joystick = nullptr;
    }

    // Reset connection state and all input values
    m_connected = false;
    m_deviceName.clear();
    m_steering = 0.0;
    m_throttle = 0.0;

    // Notify QML of all property changes
    emit connectedChanged();
    emit deviceNameChanged();
    emit steeringChanged();
    emit throttleChanged();
}

/**
 * @brief Qt slot to poll the joystick for new input values
 *
 * Called periodically by m_pollTimer (every 16ms for ~60Hz updates).
 * Reads raw axis values from SDL, normalizes them, and emits change signals
 * if values have changed by more than a small threshold (0.001 to avoid noise).
 *
 * Axis mapping:
 * - m_steeringAxis (default 0): Steering wheel position
 * - m_throttleAxis (default 2): Throttle/brake pedal position
 *
 * Special handling for throttle:
 * - Converts from SDL's -1.0..1.0 range to 0.0..1.0 range
 * - Inverts value so -1.0 (pedal fully pressed) = 1.0 (full throttle)
 */
void SteeringController::pollJoystick()
{
    // Only poll if joystick is actually connected
    if (!m_joystick) return;

    // Update SDL's internal joystick state (reads latest values from device)
    SDL_JoystickUpdate();

    // Get the number of axes this joystick has
    int numAxes = SDL_JoystickNumAxes(m_joystick);

    // Read steering axis (typically axis 0)
    if (m_steeringAxis < numAxes) {
        // Get raw axis value from SDL (typically -32768 to 32767)
        int rawSteering = SDL_JoystickGetAxis(m_joystick, m_steeringAxis);

        // Normalize to -1.0 (full left) to 1.0 (full right)
        qreal newSteering = normalizeAxis(rawSteering);

        // Only emit signal if value changed significantly (avoids noise/jitter)
        if (qAbs(newSteering - m_steering) > 0.001) {
            m_steering = newSteering;
            emit steeringChanged();
        }
    }

    // Read throttle axis (typically axis 2 for pedals)
    if (m_throttleAxis < numAxes) {
        // Get raw axis value from SDL (typically -32768 to 32767)
        int rawThrottle = SDL_JoystickGetAxis(m_joystick, m_throttleAxis);

        // Convert from -1.0..1.0 to 0.0..1.0 range
        // Invert so that pedal fully pressed (-1.0) = full throttle (1.0)
        // Formula: (1.0 - normalized) / 2.0
        // -1.0 -> (1.0 - (-1.0)) / 2.0 = 1.0 (full throttle)
        //  0.0 -> (1.0 - 0.0) / 2.0 = 0.5 (half throttle)
        //  1.0 -> (1.0 - 1.0) / 2.0 = 0.0 (no throttle)
        qreal newThrottle = (1.0 - normalizeAxis(rawThrottle)) / 2.0;

        // Only emit signal if value changed significantly
        if (qAbs(newThrottle - m_throttle) > 0.001) {
            m_throttle = newThrottle;
            emit throttleChanged();
        }
    }
}

/**
 * @brief Normalizes a raw axis value to -1.0 to 1.0 range
 * @param value Raw axis value from SDL (typically -32768 to 32767 for 16-bit axes)
 * @param min Minimum expected value (default: -32768 for 16-bit signed integer)
 * @param max Maximum expected value (default: 32767 for 16-bit signed integer)
 * @return qreal Normalized value in range -1.0 to 1.0
 *
 * Converts SDL's raw integer axis values into normalized floating-point values
 * suitable for use in game logic and UI display. Clamps input to min/max range
 * to handle any out-of-range values.
 *
 * Formula:
 * 1. Clamp value to [min, max] range
 * 2. Shift to start at 0: (value - min)
 * 3. Scale to 0..1: / (max - min)
 * 4. Scale to 0..2: * 2.0
 * 5. Shift to -1..1: - 1.0
 *
 * Example: value=0, min=-32768, max=32767
 * -> (0 - (-32768)) / 65535 * 2.0 - 1.0 ≈ 0.0 (centered)
 */
qreal SteeringController::normalizeAxis(int value, int min, int max)
{
    // Clamp value to expected range (handles any out-of-bounds values)
    if (value < min) value = min;
    if (value > max) value = max;

    // Calculate the total range (e.g., for 16-bit: 32767 - (-32768) = 65535)
    qreal range = static_cast<qreal>(max - min);

    // Normalize to -1.0 to 1.0 range
    // Step 1: Shift to 0..range by subtracting min
    // Step 2: Divide by range to get 0..1
    // Step 3: Multiply by 2 to get 0..2
    // Step 4: Subtract 1 to get -1..1
    qreal normalized = (static_cast<qreal>(value - min) / range) * 2.0 - 1.0;

    return normalized;
}
