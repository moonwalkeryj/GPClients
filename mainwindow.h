#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QtXml>
#include <QVector>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QUdpSocket>

#include <opencv2/opencv.hpp>

#include "gpthread.h"
#include "gpudp.h"
namespace Ui {
class MainWindow;
}
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    cv::Mat preOrder(QDomElement node, bool testing = false);

private:
    void initPlotingViews();
    int drawPoints(cv::Mat& dst, const cv::Mat& mask);
    int evaluateGeno(QString &textGeno);
    void initSocket();

public slots:
    // set image features
    void initImgFeatures();
    // set evolving targets with points
    void initTargetPts();
    void readPendingDatagrams();
private slots:
    void on_pushButton_clicked();

    void on_actionStart_triggered();

private:
    // evolving thread
    GPThread _thread;
    // Network Socket
    QUdpSocket *receiver;
    QUdpSocket *sender;

    // plotting data for generations' statistics
    QVector<double> _vcGenerations;
    QVector<double> _vcStd;
    QVector<double> _vcMax;
    QVector<double> _vcAvg;

    // plotting data for individual's statistics
    QVector<double> _vcThreshold;
    QVector<double> _vcFitness;
    QVector<double> _vcRecall;
    QVector<double> _vcPrecision;

    // features
    cv::Mat_<float> txx, txy, txz, tyy, tyz, tzz;
    cv::Mat_<float> _features[18];
    int _cols, _rows;

    // normal distribution array
    float _iNorDist[31];

    // evolving targets
    cv::Mat _matTargetPts;
    int _iNumTargetPts;

    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
