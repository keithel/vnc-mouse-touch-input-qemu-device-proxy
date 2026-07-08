#ifndef RFBPOINTEREVENTPACKET_H
#define RFBPOINTEREVENTPACKET_H

#include <QByteArray>

class RfbPointerEventPacket : public QByteArray
{
public:
    RfbPointerEventPacket();
    void setPointerData(quint16 x, quint16 y, quint8 buttonMask);

    inline quint16 x() const { return m_x; }
    inline quint16 y() const { return m_y; }
    inline quint8 buttonMask() const { return m_buttonMask; }

private:
    quint16 m_x = 0;
    quint16 m_y = 0;
    quint8 m_buttonMask = 0;
};

QDebug operator<<(QDebug dbg, const RfbPointerEventPacket &obj);

#endif // RFBPOINTEREVENTPACKET_H
