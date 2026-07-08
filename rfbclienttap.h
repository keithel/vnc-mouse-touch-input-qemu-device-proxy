#ifndef RFBCLIENTTAP_H
#define RFBCLIENTTAP_H

#include <QObject>

class RfbClientTap : public QObject
{
    Q_OBJECT

    enum class Phase { Version, SecuritySelect, ClientInit, Normal };

public:
    RfbClientTap(QObject *parent = nullptr);

public slots:
    void reset();
    void feed(const QByteArray &bytes);

signals:
    void pointerEvent(quint16 x, quint16 y, quint8 buttonMask);

private:
    int messageLength() const;
    bool step();

    QByteArray m_buf;
    Phase m_phase = Phase::Version;
    bool m_tapping = true;
};

#endif // RFBCLIENTTAP_H
