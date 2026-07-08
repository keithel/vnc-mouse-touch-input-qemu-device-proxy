#include "vncsessionbroker.h"
#include <iostream>

VncSessionBroker::VncSessionBroker(QTcpSocket *viewerSocketParent, quint16 deviceVncPort, quint16 deviceUartPort)
    : QObject{viewerSocketParent}
    , m_viewerSocket(viewerSocketParent)
    , m_deviceVncPort(deviceVncPort)
    , m_deviceUartPort(deviceUartPort)
    , m_deviceUartSocket(new QTcpSocket(this))
    , m_deviceVncSocket(new QTcpSocket(this))
    , m_tap(new RfbClientTap(this))
{
    m_deviceUartConnectedConnection = connect(m_deviceUartSocket, &QTcpSocket::connected, this, [this, deviceUartPort]() {
        qDebug() << "Connected to device UART port" << deviceUartPort << "to deliver pointer events.";
    });
    connect(m_deviceUartSocket, &QAbstractSocket::errorOccurred, this, [](QAbstractSocket::SocketError err) {
        qWarning() << "Device UART socket error:" << err;
    });
    m_deviceUartSocketStateConnection = connect(m_deviceUartSocket, &QTcpSocket::stateChanged, this, [this](QAbstractSocket::SocketState state) {
        emit deviceUartSocketStateChanged(state);
    });
    m_deviceUartSocket->connectToHost(QHostAddress("127.0.0.1"), m_deviceUartPort, QTcpSocket::OpenModeFlag::WriteOnly);

    m_deviceVncSocketStateConnection = connect(m_deviceVncSocket, &QTcpSocket::stateChanged, this, [this](QAbstractSocket::SocketState state) {
        emit deviceVncSocketStateChanged(state);
    });
    connect(m_deviceVncSocket, &QAbstractSocket::errorOccurred, this, [](QAbstractSocket::SocketError err) {
        qWarning() << "Device VNC socket error:" << err;
    });
    m_deviceVncSocket->connectToHost(QHostAddress("127.0.0.1"), m_deviceVncPort);

    m_viewerReadyReadConnection = connect(m_viewerSocket, &QTcpSocket::readyRead, this, &VncSessionBroker::viewerToDevicePump);
    m_viewerDisconnectedConnection = connect(m_viewerSocket, &QTcpSocket::disconnected, this, &VncSessionBroker::handleViewerDisconnected);

    m_deviceReadyReadConnection = connect(m_deviceVncSocket, &QTcpSocket::readyRead, this, &VncSessionBroker::deviceToViewerPump);
    m_deviceDisconnectedConnection = connect(m_deviceVncSocket, &QTcpSocket::disconnected, this, &VncSessionBroker::handleDeviceDisconnected);

    connect(m_tap, &RfbClientTap::pointerEvent, this, &VncSessionBroker::sendUartTouchPacket);
    // m_tap->reset();
}

VncSessionBroker::~VncSessionBroker()
{
    disconnect();
}

void VncSessionBroker::disconnect()
{
    // Close all sockets.
    QObject::disconnect(m_viewerReadyReadConnection);
    QObject::disconnect(m_viewerDisconnectedConnection);
    QObject::disconnect(m_deviceReadyReadConnection);
    QObject::disconnect(m_deviceUartConnectedConnection);

    if (m_deviceVncSocket)
        m_deviceVncSocket->disconnectFromHost();
    if (m_deviceUartSocket)
        m_deviceUartSocket->disconnectFromHost();

    // emit disconnected(); // handleViewerDisconnected emits this
}

void VncSessionBroker::viewerToDevicePump()
{
    std::cout << ">";
    std::cout.flush();

    if (!m_viewerSocket || !m_deviceVncSocket)
        return;

    QByteArray data = m_viewerSocket->readAll();
    m_deviceVncSocket->write(data);
    m_tap->feed(data);
}

void VncSessionBroker::deviceToViewerPump()
{
    std::cout << "<";
    std::cout.flush();

    if (!m_viewerSocket || !m_deviceVncSocket) {
        QString errStr;
        if (!m_viewerSocket)
            errStr = "viewerSocket";
        if (!m_viewerSocket && !m_deviceVncSocket)
            errStr.append(" and ");
        if (!m_deviceVncSocket)
            errStr.append("deviceVncSocket");
        errStr.append((!m_viewerSocket && !m_deviceVncSocket) ? " are" : " is");
        errStr.append(" disconnected");

        qDebug() << "Device to Viewer pump called when" << errStr;
        return;
    }

    QByteArray data = m_deviceVncSocket->readAll();
    m_viewerSocket->write(data);
}

void VncSessionBroker::handleViewerDisconnected()
{
    qDebug() << "Viewer disconnected.";
    if (m_deviceVncSocket)
        m_deviceVncSocket->disconnectFromHost();

    emit disconnected();
}

void VncSessionBroker::handleDeviceDisconnected()
{
    qDebug() << "Device VNC disconnected.";
    if (m_viewerSocket)
        m_viewerSocket->disconnectFromHost();
}

void VncSessionBroker::sendUartTouchPacket(quint16 x, quint16 y, quint8 buttons)
{
    if (!m_deviceUartSocket || !m_deviceUartSocket->isOpen())
        return;

    RfbPointerEventPacket packet;
    packet.setPointerData(x, y, buttons);
    m_deviceUartSocket->write(packet);

    qDebug() << "Forwarded pointer event via UART ->" << packet;
}
