#ifndef QMLQABSTRACTSOCKETFOREIGN_H
#define QMLQABSTRACTSOCKETFOREIGN_H

#include <QAbstractSocket>
#include <qqmlintegration.h>

struct QmlQAbstractSocketForeign
{
    Q_GADGET
    QML_FOREIGN(QAbstractSocket)
    QML_NAMED_ELEMENT(QAbstractSocket)
    QML_UNCREATABLE("Cannot instantiate QAbstractSocket in QML")
};

#endif // QMLQABSTRACTSOCKETFOREIGN_H
