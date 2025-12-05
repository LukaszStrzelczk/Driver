#ifndef STEERINGCONTROLLERSERVICE_H
#define STEERINGCONTROLLERSERVICE_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include "../../src/includes/steeringcontroller.hpp"

class SteeringControllerService : public QObject
{
    Q_OBJECT
public:
    explicit SteeringControllerService(SteeringController * controller, QObject *parent=nullptr);
    ~SteeringControllerService();

    // connection
    void connectToServer(const QString &url);
    void disconnect();
    bool isConnected() const;

signals: 
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

    void onSteeringDataChanged();

private:
    QWebSocket *m_webSocket;
    SteeringController *m_controller;
    bool m_isConnected;

    //private methods
    void sendSteeringData();
    QJsonObject createDataPayload() const;
};

#endif // STEERINGCONTROLLERSERVICE_H
