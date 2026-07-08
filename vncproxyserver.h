#ifndef VNCPROXYSERVER_H
#define VNCPROXYSERVER_H

#include <QTcpServer>
#include <QQmlEngine>
#include <vncsessionbroker.h>

class VncProxyServer : public QTcpServer
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int proxyPort READ proxyPort WRITE setProxyPort NOTIFY proxyPortChanged FINAL)
    Q_PROPERTY(int vncPort READ vncPort WRITE setVncPort NOTIFY vncPortChanged FINAL)
    Q_PROPERTY(int uartPort READ uartPort WRITE setUartPort NOTIFY uartPortChanged FINAL)
    Q_PROPERTY(bool listening READ listening NOTIFY listeningChanged FINAL)
    Q_PROPERTY(bool viewerConnected READ viewerConnected NOTIFY viewerConnectedChanged FINAL)
    Q_PROPERTY(VncSessionBroker* sessionBroker READ sessionBroker NOTIFY sessionBrokerChanged FINAL)

public:
    VncProxyServer();
    virtual ~VncProxyServer();

    Q_INVOKABLE void start();

    inline int proxyPort() const { return m_proxyPort; }
    inline int vncPort() const { return m_vncPort; }
    inline int uartPort() const { return m_uartPort; }
    inline bool listening() const { return m_listening; }
    inline bool viewerConnected() const { return m_viewerSocket != nullptr; }
    inline VncSessionBroker *sessionBroker() const { return m_sessionBroker; }

    void setProxyPort(int newProxyPort);
    void setVncPort(int newVncPort);
    void setUartPort(int newUartPort);

signals:
    void proxyPortChanged(int val);
    void vncPortChanged(int val);
    void uartPortChanged(int val);
    void listeningChanged(bool val);
    void viewerConnectedChanged(bool val);
    void sessionBrokerChanged();

public slots:
    void handleNewConnection();

private:
    int m_proxyPort = 5901;
    int m_vncPort = 5900;
    int m_uartPort = 12345;
    bool m_listening = false;
    VncSessionBroker *m_sessionBroker = nullptr;

    QTcpSocket *m_viewerSocket = nullptr;
    QMetaObject::Connection m_listenConnection;
};

#endif // VNCPROXYSERVER_H
