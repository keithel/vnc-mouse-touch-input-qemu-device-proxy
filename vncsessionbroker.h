#ifndef VNCSESSIONBROKER_H
#define VNCSESSIONBROKER_H

#include <QObject>
#include <QQmlEngine>
#include <QTcpSocket>

#include "rfbpointereventpacket.h"
#include "rfbclienttap.h"

class VncSessionBroker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Uncreatable in QML. Access through VncProxyServer")
    Q_PROPERTY(QAbstractSocket::SocketState deviceUartSocketState READ deviceUartSocketState NOTIFY deviceUartSocketStateChanged FINAL)
    Q_PROPERTY(QAbstractSocket::SocketState deviceVncSocketState READ deviceVncSocketState NOTIFY deviceVncSocketStateChanged FINAL)

public:
    explicit VncSessionBroker(QTcpSocket *viewerSocketParent, quint16 deviceVncPort, quint16 deviceUartPort);
    virtual ~VncSessionBroker();
    void disconnect();

    inline QAbstractSocket::SocketState deviceUartSocketState() const { return m_deviceUartSocket ? m_deviceUartSocket->state() : QAbstractSocket::UnconnectedState; }
    inline QAbstractSocket::SocketState deviceVncSocketState() const { return m_deviceVncSocket ? m_deviceVncSocket->state() : QAbstractSocket::UnconnectedState; }

signals:
    void deviceUartSocketStateChanged(QAbstractSocket::SocketState state);
    void deviceVncSocketStateChanged(QAbstractSocket::SocketState state);
    void disconnected();

private slots:
    void viewerToDevicePump();
    void deviceToViewerPump();
    void handleViewerDisconnected();
    void handleDeviceDisconnected();

private:
    void sendUartTouchPacket(quint16 x, quint16 y, quint8 buttons);

private:
    QTcpSocket *m_viewerSocket = nullptr;
    quint16 m_deviceVncPort = 0;
    quint16 m_deviceUartPort = 0;
    QPointer<QTcpSocket> m_deviceUartSocket = nullptr;
    QPointer<QTcpSocket> m_deviceVncSocket = nullptr;
    QPointer<RfbClientTap> m_tap = nullptr;

    QMetaObject::Connection m_deviceUartConnectedConnection;
    QMetaObject::Connection m_deviceUartSocketStateConnection;
    QMetaObject::Connection m_deviceVncSocketStateConnection;
    QMetaObject::Connection m_viewerReadyReadConnection;
    QMetaObject::Connection m_viewerDisconnectedConnection;
    QMetaObject::Connection m_deviceReadyReadConnection;
    QMetaObject::Connection m_deviceDisconnectedConnection;
};

#endif // VNCSESSIONBROKER_H
