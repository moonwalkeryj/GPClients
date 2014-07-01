#include <QThread>

class GPThread : public QThread
{
    Q_OBJECT
private:


public:
    void run();
public slots:
    void stop();
};
