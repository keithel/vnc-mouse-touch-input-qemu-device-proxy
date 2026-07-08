#include "rfbpointereventpacket.h"
#include <QDebug>

RfbPointerEventPacket::RfbPointerEventPacket()
    : QByteArray()
{
}

void RfbPointerEventPacket::setPointerData(quint16 x, quint16 y, quint8 buttonMask)
{
    m_x = x;
    m_y = y;
    m_buttonMask = buttonMask;

    clear();
    append(static_cast<char>(0xAA)); // Sync byte
    append(static_cast<char>(buttonMask));
    append(static_cast<char>((x >> 8) & 0xFF));
    append(static_cast<char>(x & 0xFF));
    append(static_cast<char>((y >> 8) & 0xFF));
    append(static_cast<char>(y & 0xFF));
}

QDebug operator<<(QDebug dbg, const RfbPointerEventPacket &obj)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "RfbPointerEventPacket(" << obj.x() << ", " << obj.y() << ", " << obj.buttonMask() << ")";
    return dbg;
}