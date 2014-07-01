#include "gpudp.h"
#include <iostream>
GPUDP::GPUDP(QObject *parent)
{
}
void GPUDP::initSocket()
{
    receiver = new QUdpSocket(this);
    receiver->bind(QHostAddress::LocalHost, 3001);

    sender = new QUdpSocket(this);
    //sender->bind(QHostAddress::LocalHost, 3002);

    connect(receiver, SIGNAL(readyRead()),
            this, SLOT(readPendingDatagrams()));

    std::cout << "Good" << std::endl;
}

void GPUDP::readPendingDatagrams()
{
    std::cout << "read pending" << std::endl;
    while (receiver->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(receiver->pendingDatagramSize());
        QHostAddress senderAddr;
        quint16 senderPort;

        receiver->readDatagram(datagram.data(), datagram.size(),
                                &senderAddr, &senderPort);

        //processTheDatagram(datagram);
        QString fitness = QString::number(56);
        //fitness = "\"" + fitness + "\"";
        QByteArray msg = fitness.toAscii();
        sender->writeDatagram(msg, msg.size(), QHostAddress::LocalHost, 3002);
        std::cout << msg.data() << std::endl;
    }
}
