/**
 * @file steeringcontroller.hpp
 * @brief Header file for steering wheel and game controller input handling using SDL2
 *
 * This file defines the SteeringController class which interfaces with physical
 * steering wheels, pedals, and game controllers through the SDL2 library. It provides
 * normalized input values (steering and throttle) to QML for driving applications.
 */

#ifndef STEERINGCONTROLLER_H
#define STEERINGCONTROLLER_H

// Qt includes for core functionality and QML integration
#include <QObject>       // Base class for Qt objects with signal/slot support
#include <QQmlEngine>    // QML engine integration for exposing to QML
#include <QTimer>        // Timer for periodic joystick polling

// SDL2 includes for game controller/joystick support
#include <SDL2/SDL.h>    // Simple DirectMedia Layer for input device handling

// Standard library includes
#include <vector>        // For storing device indices

/**
 * @class SteeringController
 * @brief Manages steering wheel and game controller input using SDL2
 *
 * This class provides an interface for connecting to and reading input from
 * steering wheels, pedals, and game controllers. It uses SDL2's joystick API
 * to detect devices and read axis values, which are then normalized to a
 * -1.0 to 1.0 range for easy use in QML applications.
 *
 * Features:
 * - Auto-detection of connected input devices
 * - Real-time polling of joystick axes (steering and throttle)
 * - Normalized input values for consistent behavior across devices
 * - QML integration through Q_PROPERTY declarations
 * - Device connection/disconnection management
 *
 * Typical usage:
 * 1. Call refreshDevices() to scan for available devices
 * 2. Call connectDevice(index) to connect to a specific device
 * 3. Monitor steering and throttle properties in QML
 * 4. Call disconnectDevice() when done
 */
class SteeringController : public QObject
{
    Q_OBJECT  // Enable Qt's meta-object system (signals/slots, properties, etc.)
    QML_ELEMENT  // Make this class available to QML as a singleton or instantiable type

    // QML-accessible properties with getters and change notification signals
    Q_PROPERTY(qreal steering READ steering NOTIFY steeringChanged)              ///< Normalized steering value (-1.0 = full left, 1.0 = full right)
    Q_PROPERTY(qreal throttle READ throttle NOTIFY throttleChanged)              ///< Normalized throttle value (-1.0 = full brake, 1.0 = full throttle)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)            ///< Whether a device is currently connected
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)      ///< Name of the currently connected device
    Q_PROPERTY(QStringList availableDevices READ availableDevices NOTIFY availableDevicesChanged)  ///< List of detected device names

public:
    /**
     * @brief Constructs the steering controller and initializes SDL2
     * @param parent Parent QObject for memory management (follows Qt parent-child pattern)
     */
    explicit SteeringController(QObject *parent = nullptr);

    /**
     * @brief Destructor - ensures device is disconnected and SDL2 is cleaned up
     */
    ~SteeringController();

    // Property getters for QML access

    /**
     * @brief Returns the current normalized steering value
     * @return qreal value from -1.0 (full left) to 1.0 (full right), 0.0 is center
     */
    qreal steering() const { return m_steering; }

    /**
     * @brief Returns the current normalized throttle value
     * @return qreal value from -1.0 (full brake) to 1.0 (full throttle), 0.0 is neutral
     */
    qreal throttle() const { return m_throttle; }

    /**
     * @brief Returns whether a device is currently connected
     * @return true if device is connected and polling, false otherwise
     */
    bool connected() const { return m_connected; }

    /**
     * @brief Returns the name of the currently connected device
     * @return QString containing device name, or empty string if not connected
     */
    QString deviceName() const { return m_deviceName; }

    /**
     * @brief Returns list of all detected input devices
     * @return QStringList with names of available devices
     */
    QStringList availableDevices() const { return m_availableDevices; }

    // QML-invokable methods (callable from JavaScript in QML)

    /**
     * @brief Scans for and updates the list of available input devices
     *
     * This method queries SDL2 for all connected joystick/game controller devices
     * and updates the availableDevices property. Should be called when the user
     * wants to see what devices are connected or after plugging in a new device.
     */
    Q_INVOKABLE void refreshDevices();

    /**
     * @brief Connects to a specific input device by index
     * @param index Index into the availableDevices list (0-based)
     *
     * Opens the specified joystick device and begins polling its axes for input.
     * If another device is already connected, it will be disconnected first.
     * Emits connectedChanged and deviceNameChanged signals on success.
     */
    Q_INVOKABLE void connectDevice(int index);

    /**
     * @brief Disconnects the currently connected input device
     *
     * Stops polling and closes the joystick device. Safe to call even if
     * no device is connected. Emits connectedChanged signal.
     */
    Q_INVOKABLE void disconnectDevice();

signals:
    // Qt signals emitted when properties change (for QML property bindings)

    /**
     * @brief Emitted when the steering value changes
     */
    void steeringChanged();

    /**
     * @brief Emitted when the throttle value changes
     */
    void throttleChanged();

    /**
     * @brief Emitted when device connection state changes (connected/disconnected)
     */
    void connectedChanged();

    /**
     * @brief Emitted when the connected device name changes
     */
    void deviceNameChanged();

    /**
     * @brief Emitted when the list of available devices changes
     */
    void availableDevicesChanged();

private slots:
    /**
     * @brief Qt slot to poll the joystick for new input values
     *
     * Called periodically by m_pollTimer (typically every 16ms for ~60Hz polling).
     * Reads axis values from the connected joystick, normalizes them, and emits
     * change signals if values have changed.
     */
    void pollJoystick();

private:
    // Private helper methods

    /**
     * @brief Initializes the SDL2 library subsystems
     *
     * Called in constructor to set up SDL2's joystick subsystem.
     * Must be called before any SDL joystick functions can be used.
     */
    void initSDL();

    /**
     * @brief Cleans up and shuts down SDL2 subsystems
     *
     * Called in destructor to properly shut down SDL2 and free resources.
     * Ensures the joystick subsystem is properly closed.
     */
    void cleanupSDL();

    /**
     * @brief Scans for connected devices and updates internal device lists
     *
     * Queries SDL2 for all joystick devices, populates m_availableDevices
     * with device names and m_deviceIndices with their SDL indices.
     * Called by refreshDevices() and during initialization.
     */
    void updateDeviceList();

    /**
     * @brief Opens a specific joystick device and starts polling
     * @param index SDL device index (from m_deviceIndices, not list position)
     *
     * Opens the SDL joystick device, determines which axes to use for
     * steering and throttle (typically axis 0 and 1), and starts the
     * polling timer. Updates connection state and device name.
     */
    void openJoystick(int index);

    /**
     * @brief Closes the currently open joystick device
     *
     * Stops the polling timer and closes the SDL joystick handle.
     * Safe to call even if no joystick is open. Updates connection state.
     */
    void closeJoystick();

    /**
     * @brief Normalizes a raw axis value to -1.0 to 1.0 range
     * @param value Raw axis value from SDL (typically -32768 to 32767)
     * @param min Minimum expected value (default: -32768)
     * @param max Maximum expected value (default: 32767)
     * @return qreal Normalized value in range -1.0 to 1.0
     *
     * Converts raw integer axis values from SDL into normalized floating-point
     * values suitable for use in game logic and UI display.
     */
    qreal normalizeAxis(int value, int min = -32768, int max = 32767);

    // Member variables (all prefixed with m_ following Qt convention)

    // SDL joystick handling
    SDL_Joystick *m_joystick;  ///< SDL joystick handle for the connected device (nullptr if not connected)
    QTimer *m_pollTimer;       ///< Timer that triggers periodic polling of joystick axes

    // Current input values (normalized to -1.0 to 1.0 range)
    qreal m_steering;  ///< Current steering wheel position (-1.0 = full left, 1.0 = full right)
    qreal m_throttle;  ///< Current throttle/brake position (-1.0 = full brake, 1.0 = full throttle)

    // Device connection state
    bool m_connected;                      ///< Flag indicating whether a device is currently connected
    QString m_deviceName;                  ///< Name of the currently connected device
    QStringList m_availableDevices;        ///< User-friendly list of device names for UI display
    std::vector<int> m_deviceIndices;      ///< Corresponding SDL device indices (parallel to m_availableDevices)

    // Axis mapping configuration
    int m_steeringAxis;  ///< Which joystick axis to use for steering (typically 0)
    int m_throttleAxis;  ///< Which joystick axis to use for throttle (typically 1)
};

#endif // STEERINGCONTROLLER_H