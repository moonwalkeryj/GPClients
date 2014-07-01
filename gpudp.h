#ifndef GPUDP_H
#define GPUDP_H

#include <QUdpSocket>

class GPUDP : public QObject
{
    Q_OBJECT
private:
    QUdpSocket *receiver;
    QUdpSocket *sender;
public:
    explicit GPUDP(QObject *parent = 0);
    void initSocket();

    
signals:
    
public slots:
    void readPendingDatagrams();
};

#endif // GPUDP_H
