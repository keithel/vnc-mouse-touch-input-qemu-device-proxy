#include "rfbclienttap.h"
#include <QDebug>

namespace {
enum RfbClientMsg : quint8 {
    SetPixelFormat = 0,
    SetEncodings = 2,
    FrameBufferUpdateRequest = 3,
    KeyEvent = 4,
    PointerEvent = 5,
    ClientCutText = 6
};

quint16 be16(const QByteArray &b, int i) {
    return (quint8(b[i]) << 8) | quint8(b[i + 1]); }
quint32 be32(const QByteArray &b, int i) {
    return (quint32(quint8(b[i])) << 24)
           | (quint8(b[i + 1]) << 16)
           | (quint8(b[i + 2]) << 8)
           | quint8(b[i + 3]);
}
} // anonymous namespace

RfbClientTap::RfbClientTap(QObject *parent) {}

void RfbClientTap::reset()
{
    m_buf.clear();
    m_phase = Phase::Version;
    m_tapping = true;
}

void RfbClientTap::feed(const QByteArray &bytes)
{
    if (!m_tapping)
        return;

    m_buf.append(bytes);
    // Make a smuch progress as the buffered bytes allow.
    while (step()) { }
}

int RfbClientTap::messageLength() const
{
    if (m_buf.isEmpty())
        return 0;

    switch(quint8(m_buf[0])) {
    case SetPixelFormat:            return 20; // 1 + 3 pad + 16
    case FrameBufferUpdateRequest:  return 10;
    case KeyEvent:                  return 8;
    case PointerEvent:              return 6;
    case SetEncodings:              return m_buf.size() < 4 ? 0 : (4 + 4 * be16(m_buf, 2));
    case ClientCutText:             return m_buf.size() < 8 ? 0 : (8 + int(be32(m_buf, 4)));
    default:                        return -1; // Unknown type / extension.
    }
}

bool RfbClientTap::step()
{
    switch (m_phase) {
    case Phase::Version:
        if (m_buf.size() < 12)
            return false;
        {
            // "RFB 003.00x\n": minor >= 7 means a security-type byte follows
            bool ok = true;
            const int minor = m_buf.mid(8, 3).toInt(&ok);
            if (!ok) {
                qWarning() << "Unexpected version" << m_buf.mid(8, 3);
                return false;
            }
            m_buf.remove(0, 12);
            m_phase = (minor >= 7) ? Phase::SecuritySelect : Phase::ClientInit;
        }
        return true;

    case Phase::SecuritySelect:
        if (m_buf.isEmpty())
            return false;

        m_buf.remove(0, 1);
        m_phase = Phase::ClientInit;
        return true;

    case Phase::ClientInit:
        if (m_buf.isEmpty())
            return false;

        m_buf.remove(0, 1);
        m_phase = Phase::Normal;
        return true;

    case Phase::Normal:
    {
        const int n = messageLength();
        if (n == 0)
            return false;

        if (n < 0) {
            m_tapping = false;
            m_buf.clear();
            return false;
        }

        if (m_buf.size() < n)
            return false;

        if (quint8(m_buf[0]) == PointerEvent) {
            emit pointerEvent(be16(m_buf, 2), be16(m_buf, 4), quint8(m_buf[1]));
        }
        m_buf.remove(0, n);
        return true;
    }
    }
    return false;
}