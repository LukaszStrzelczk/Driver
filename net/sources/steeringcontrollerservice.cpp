#include "includes/steeringcontrollerservice.hpp"
#include "../../src/includes/steeringcontroller.hpp"
#include <QDebug>

SteeringControllerService::SteeringControllerService(SteeringController *controller, QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_controller(controller)
    , m_isConnected(false)
{
    connect(m_webSocket, &QWebSocket::connected, this, &SteeringControllerService::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &SteeringControllerService::onDisconnected);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &SteeringControllerService::onError);

    if (m_controller) {
        connect(m_controller, &SteeringController::steeringChanged,
                this, &SteeringControllerService::onSteeringDataChanged);
        connect(m_controller, &SteeringController::throttleChanged,
                this, &SteeringControllerService::onSteeringDataChanged);
        connect(m_controller, &SteeringController::connectedChanged,
                this, &SteeringControllerService::onSteeringDataChanged);
    }
}

SteeringControllerService::~SteeringControllerService()
{
    if (m_webSocket) {
        m_webSocket->close();
    }
}
void SteeringControllerService::connectToServer(const QString &url)
{
    if (m_isConnected) {
        qWarning() << "Already connected to WebSocket server";
        return;
    }

    qDebug() << "Connecting to WebSocket server:" << url;
    m_webSocket->open(QUrl(url));
}

void SteeringControllerService::disconnect()
{
    if (m_webSocket && m_isConnected) {
        qDebug() << "Disconnecting from WebSocket server";
        m_webSocket->close();
    }
}

bool SteeringControllerService::isConnected() const
{
    return m_isConnected;
}

void SteeringControllerService::onConnected()
{
    m_isConnected = true;
    qDebug() << "WebSocket connected to:" << m_webSocket->requestUrl();
    emit connected();

    // Send initial state
    sendSteeringData();
}

void SteeringControllerService::onDisconnected()
{
    m_isConnected = false;
    qDebug() << "WebSocket disconnected";
    emit disconnected();
}

void SteeringControllerService::onError(QAbstractSocket::SocketError error)
{
    QString errorString = m_webSocket->errorString();
    qWarning() << "WebSocket error:" << error << "-" << errorString;
    emit errorOccurred(errorString);
}

void SteeringControllerService::onSteeringDataChanged()
{
    // Only send data if connected
    if (m_isConnected) {
        sendSteeringData();
    }
}

void SteeringControllerService::sendSteeringData()
{
    if (!m_controller || !m_isConnected) {
        return;
    }

    QJsonObject packet = createDataPayload();
    QJsonDocument doc(packet);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    m_webSocket->sendTextMessage(jsonString);
}

QJsonObject SteeringControllerService::createDataPayload() const
{
    QJsonObject packet;

    if (m_controller) {
        packet["angle"] = m_controller->steering();
        packet["throttle"] = m_controller->throttle();
        packet["drive_mode"] = QString("user");
        packet["recording"] = false;
    }

    return packet;
}