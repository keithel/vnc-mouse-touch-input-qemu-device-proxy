#include "vncproxyserver.h"
#include <QDebug>
#include <algorithm>
#include <QTimer>

VncProxyServer::VncProxyServer()
{
    qDebug() << "VncProxyServer instantiated, proxyPort" << m_proxyPort << "vncPort" << m_vncPort << "uartPort" << m_uartPort;
}

VncProxyServer::~VncProxyServer()
{
    if(m_viewerSocket)
        m_viewerSocket->close();
    disconnect(m_listenConnection);
    m_listening = false;

}

void VncProxyServer::start()
{
    bool isListening = VncProxyServer::isListening();
    if ((isListening && !m_listening) || (!isListening && m_listening)) {
        qCritical() << "Something went wrong. VncProxyServer::isListening and VncProxyServer::m_listening don't match.";
        return;
    }

    if (isListening) {
        qWarning() << "VncProxyServer::start() already called. Ignoring.";
        return;
    }

    m_listenConnection = connect(this, &QTcpServer::newConnection, this, &VncProxyServer::handleNewConnection);
    if(listen(QHostAddress::LocalHost, m_proxyPort)) {
        qDebug() << "Proxy server listening for VNC Viewer on port" << m_proxyPort;
        m_listening = true;
        emit listeningChanged(m_listening);
    }
    else {
        disconnect(m_listenConnection);
        qCritical() << "Failed to start proxy server:" << errorString();
    }
}

void VncProxyServer::setProxyPort(int newProxyPort)
{
    int clampedNewValue = std::clamp(newProxyPort, 0, 65535);
    if (m_proxyPort == clampedNewValue)
        return;
    m_proxyPort = clampedNewValue;
    qDebug() << "proxyPort changed:" << m_proxyPort;
    emit proxyPortChanged(m_proxyPort);
}

void VncProxyServer::setVncPort(int newVncPort)
{
    int clampedNewValue = std::clamp(newVncPort, 0, 65535);
    if (m_vncPort == clampedNewValue)
        return;
    m_vncPort = clampedNewValue;
    qDebug() << "vncPort changed:" << m_vncPort;
    emit vncPortChanged(m_vncPort);
}

void VncProxyServer::setUartPort(int newUartPort)
{
    int clampedNewValue = std::clamp(newUartPort, 0, 65535);
    if (m_uartPort == clampedNewValue)
        return;
    m_uartPort = clampedNewValue;
    qDebug() << "uartPort changed:" << m_uartPort;
    emit uartPortChanged(m_uartPort);
}

void VncProxyServer::handleNewConnection()
{
    if (m_viewerSocket) {
        m_viewerSocket->disconnectFromHost();
        m_viewerSocket = nullptr;
        // emit viewerConnectedChanged(false);
    }

    if (m_sessionBroker) {
        m_sessionBroker->disconnect();
        m_sessionBroker = nullptr; // deleted when m_viewerSocket is deleted
    }

    m_viewerSocket = nextPendingConnection();
    m_sessionBroker = new VncSessionBroker(m_viewerSocket, m_vncPort, m_uartPort);
    emit sessionBrokerChanged();
    m_sessionBrokerDisconnectedConnection = connect(m_sessionBroker, &VncSessionBroker::disconnected, this, [this](){
        qDebug() << "sessionBroker disconnected.";
        m_viewerSocket->disconnectFromHost();
        if (m_sessionBroker) {
            QTimer::singleShot(0, [this](){
                m_sessionBroker.clear(); // It will be deleted when m_viewerSocket is deleted
                emit sessionBrokerChanged();
            });
        }
    });

    connect(m_viewerSocket, &QTcpSocket::disconnected, this, [this](){
        qDebug() << "VncProxyServer viewerSocket disconnected handler";

        m_viewerSocket->deleteLater();
        m_viewerSocket = nullptr;
        emit viewerConnectedChanged(false);
    });

    if (m_viewerSocket != nullptr)
        emit viewerConnectedChanged(true);

    qDebug() << "VNC Viewer connected from" << m_viewerSocket->peerAddress();
}