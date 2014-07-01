
#include "mainwindow.h"
#include "ui_mainwindow.h"

//#include <boost/shared_ptr.hpp>
//#include <boost/thread/mutex.hpp>
//#include <boost/thread/thread.hpp>
#include <QApplication>
#include <QtCore>
#include <QFileDialog>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <QtXml>
#include <QtConcurrentRun>

#include <iostream>

int countPts(const cv::Mat & img)
{
    assert(img.type() == CV_8UC1);

    cv::Size size = img.size();
    if( img.isContinuous())
    {
        size.width *= size.height;
        size.height = 1;
    }

    int result = 0;

    for( int i = 0; i < size.height; i++ )
    {
        uchar* p_img = (uchar*)(img.data + img.step*i);


        for( int j = 0; j < size.width; j++ )
        {
            if(p_img[j])
            {
                result++;
            }
        }

    }
    return result;
}

/*!
 *  \brief extract feature points from reponse image.
 *  \param result in interest points reponse mat
 *  \param ptsMask out a mask image with 255 indicate detected feature points
 */
void detectFeaturePts(const cv::Mat & result, cv::Mat & ptsMask, int threshold)
{
    ptsMask = cv::Mat::zeros(result.size(), CV_8UC1);

    // threshold to obtain feature pts
    for( int j = 0; j < result.rows ; j++ )
    {
        for( int i = 0; i < result.cols; i++ )
        {
            if( (int) result.at<short>(j,i) > threshold )
            {
                ptsMask.at<uchar>(j,i) = 255;
                //cv::circle( src, cv::Point( i, j ), 2,  cv::Scalar(255, 255, 0), 1, 8, 0 );
            }
        }
    }

    //    // non maximum suppression
    //    cv::Mat ptsMaskCopy = ptsMask.clone();
    //    int radius = 5;
    //    for( int i = 0; i < result.rows - 2*radius; i++ )
    //    {
    //        const uchar* pResult = (const uchar*)(result.data + result.step*i);
    //        uchar* pPtsMaskCopy = (uchar*)(ptsMaskCopy.data + ptsMaskCopy.step*i);
    //        uchar* pPtsMask = (uchar*)(ptsMask.data + ptsMask.step*i);
    //        for( int j; j < result.cols - 2*radius; j++ )
    //        {
    //            if(pPtsMaskCopy[j])
    //            {
    //                // judge whether it is the maximum
    //                int index = 0;
    //                if(p_mask[j+1])
    //                {
    //                    if(p_dst_norm[j] < p_dst_norm[j+1]) { p_mask[j] = 0; index = 1;}
    //                    else p_mask[j+1] = 0;
    //                }
    //                if(p_mask[j+_detected_cormsk.step])
    //                {
    //                    if(p_dst_norm[j+index] < p_dst_norm[j+dst_norm.step]) { p_mask[j+index] = 0;}
    //                    else p_mask[j+_detected_cormsk.step] = 0;
    //                }
    //            }
    //        }
    //    }

}

void calculateStatistics(const cv::Mat &detected_mask, const cv::Mat &desired_mask, int desiredTotalNumPts, double &recall, double &precision, double &fitness)
{
    int num_matched, num_response;
    bool isShow = false;

    num_response = countPts(detected_mask);

    if (isShow) {
        cv::imshow("Detected Points", detected_mask);
        cv::waitKey(1);
    }

    cv::Mat matMatchedPts = detected_mask & desired_mask;
    num_matched = countPts(matMatchedPts);

    if (isShow) {
        cv::imshow("Matched Points", matMatchedPts);
        cv::waitKey(1);
    }

    if (num_response == 0) precision = 0;
    else precision = (double)num_matched/num_response;

    recall = (double)num_matched/desiredTotalNumPts;

    fitness = (recall*precision)/(0.8*recall + 0.2*precision);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // initialize normal distribution
    // 0    1       2
    float _p[] = {0.5, 0.46017, 0.42074, 0.38209, 0.34458, 0.30854, 0.27425, 0.24196, 0.21186, 0.18406, 0.15866, 0.13567, 0.11507, 0.09680, 0.08076, 0.06681};

    for (int i=0; i < 31; i++)
    {
        int index = 15-i;
        if ( i < 16)
        {
            _iNorDist[i] = 2*_p[index];
        }
        else
        {
            _iNorDist[i] = 2*_p[i-15];
        }

        std::cout << i << ": " << _iNorDist[i] << std::endl;
    }
    // read desired feature postion
    initTargetPts();

    // initialize image features
    initImgFeatures();



    // initialize plotting views
    initPlotingViews();

    // initialize udp
    //initSocket();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initSocket()
{
    receiver = new QUdpSocket(this);
    receiver->bind(QHostAddress::LocalHost, 3001);

    sender = new QUdpSocket(this);
    //sender->bind(QHostAddress::LocalHost, 3002);

    connect(receiver, SIGNAL(readyRead()),
            this, SLOT(readPendingDatagrams()));

    std::cout << "Network initialized" << std::endl;
}

void MainWindow::readPendingDatagrams()
{
    while (receiver->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(receiver->pendingDatagramSize());
        QHostAddress senderAddr;
        quint16 senderPort;

        receiver->readDatagram(datagram.data(), datagram.size(),
                               &senderAddr, &senderPort);


        //processTheDatagram(datagram);
        int fitness = evaluateGeno(QString(datagram));
        //int fitness = 20;
        //fitness = "\"" + fitness + "\"";

        QByteArray msg = QString::number(fitness).toAscii();
        sender->writeDatagram(msg, msg.size(), QHostAddress::LocalHost, 3002);
        std::cout << msg.data() << std::endl;
    }
}

// fitness value ranging from 0 to 10000
int MainWindow::evaluateGeno(QString& textGeno)
{
    // populize domdocument
    QDomDocument doc;
    if (!doc.setContent(textGeno))
    {
        qDebug() << "Fail to populize domdocument...";
        return 0;
    }

    ui->textEdit_2->setText(textGeno);

    cv::Mat result, norm;
    qDebug() << doc.firstChildElement().tagName();
    qDebug() << doc.firstChildElement().firstChildElement().tagName();
    // get feature point detection result
    result = preOrder( doc.firstChildElement().firstChildElement() );

    if (false)
    {
        cv::imshow("result", result);
    }

    // normalizing to period [0, 255]
    cv::normalize( result, norm, -16, 16, cv::NORM_MINMAX, CV_16SC1, cv::Mat() );
    cv::Mat response;
    cv::normalize( result, response, 0, 255, cv::NORM_MINMAX, CV_8UC1, cv::Mat() );
    cv::imshow("Response", response);
    //cv::resizeWindow("Response", 256, 256);

    // clear vectors
    _vcPrecision.clear();
    _vcRecall.clear();
    _vcFitness.clear();
    _vcThreshold.clear();

    double lFitness = 0;
    int max_threshold = 0;

    // value interval [-16, 16]; threshold interval [-15, 15]
    for (int c = 0; c < 31; c++)
    {
        // set a detection threshold
        int threshold = c - 15;
        _vcThreshold.append(threshold);

        // calculate detected pts
        cv::Mat matDetectedPts;
        detectFeaturePts(norm, matDetectedPts, threshold);

        // calculate recall and precision
        double recall, precision, fitness;
        calculateStatistics(matDetectedPts, _matTargetPts, _iNumTargetPts, recall, precision, fitness);

        _vcPrecision.append(precision);
        _vcRecall.append(recall);
        _vcFitness.append(fitness);

        if (lFitness < fitness)
        {
            lFitness = fitness;
            max_threshold = c;
        }
    }

    ui->plot_individual->graph(0)->setData(_vcThreshold, _vcRecall);
    ui->plot_individual->graph(1)->setData(_vcThreshold, _vcPrecision);
    ui->plot_individual->graph(2)->setData(_vcThreshold, _vcFitness);
    ui->plot_individual->replot();
    int fitness = (int)(lFitness*_iNorDist[max_threshold]*10000);
    if (fitness == 0) fitness = 1;
    ui->statusbar->showMessage("Fitness value: " + QString::number(fitness));
    return fitness;
}

void MainWindow::initImgFeatures()
{
    //cv::FileStorage storagef0("f_0.xml", cv::FileStorage::READ); storagef0["f"] >> _f[0].getWrappedValue(); storagef0.release();
    /*for (int i = 1; i < 24; i++)
    {
        QString filename("f_");
        filename += QString::number(i);
        std::cout << filename.toStdString() << std::endl;
        cv::FileStorage file( (filename+ ".xml").toStdString(), cv::FileStorage::READ );
        int count = 0;
        while (!file.isOpened() && count < 10)
        {
            std::cout << ++count << " times to open" << std::endl;
            file.open((filename+ QString(".xml")).toStdString(), cv::FileStorage::READ);
            count++;

        }
        file["f"] >> _features[i];
        file.release();
    }*/
    cv::FileStorage storage0("f_0.xml", cv::FileStorage::READ); storage0["f"] >> _features[0]; storage0.release();
    cv::FileStorage storage1("f_1.xml", cv::FileStorage::READ); storage1["f"] >> _features[1]; storage1.release();
    cv::FileStorage storage2("f_2.xml", cv::FileStorage::READ); storage2["f"] >> _features[2]; storage2.release();
    cv::FileStorage storage3("f_3.xml", cv::FileStorage::READ); storage3["f"] >> _features[3]; storage3.release();
    cv::FileStorage storage4("f_4.xml", cv::FileStorage::READ); storage4["f"] >> _features[4]; storage4.release();
    cv::FileStorage storage5("f_5.xml", cv::FileStorage::READ); storage5["f"] >> _features[5]; storage5.release();
    cv::FileStorage storage6("f_6.xml", cv::FileStorage::READ); storage6["f"] >> _features[6]; storage6.release();
    cv::FileStorage storage7("f_7.xml", cv::FileStorage::READ); storage7["f"] >> _features[7]; storage7.release();
    cv::FileStorage storage8("f_8.xml", cv::FileStorage::READ); storage8["f"] >> _features[8]; storage8.release();
    cv::FileStorage storage9("f_9.xml", cv::FileStorage::READ); storage9["f"] >> _features[9]; storage9.release();

    cv::FileStorage storage10("f_10.xml", cv::FileStorage::READ); storage10["f"] >> _features[10]; storage10.release();
    cv::FileStorage storage11("f_11.xml", cv::FileStorage::READ); storage11["f"] >> _features[11]; storage11.release();
    cv::FileStorage storage12("f_12.xml", cv::FileStorage::READ); storage12["f"] >> _features[12]; storage12.release();
    cv::FileStorage storage13("f_13.xml", cv::FileStorage::READ); storage13["f"] >> _features[13]; storage13.release();
    cv::FileStorage storage14("f_14.xml", cv::FileStorage::READ); storage14["f"] >> _features[14]; storage14.release();
    cv::FileStorage storage15("f_15.xml", cv::FileStorage::READ); storage15["f"] >> _features[15]; storage15.release();
    cv::FileStorage storage16("f_16.xml", cv::FileStorage::READ); storage16["f"] >> _features[16]; storage16.release();
    cv::FileStorage storage17("f_17.xml", cv::FileStorage::READ); storage17["f"] >> _features[17]; storage17.release();
    //    cv::FileStorage storage18("f_18.xml", cv::FileStorage::READ); storage18["f"] >> _features[18]; storage18.release();
    //    cv::FileStorage storage19("f_19.xml", cv::FileStorage::READ); storage19["f"] >> _features[19]; storage19.release();
    //    cv::FileStorage storage20("f_20.xml", cv::FileStorage::READ); storage20["f"] >> _features[20]; storage20.release();
    //    cv::FileStorage storage21("f_21.xml", cv::FileStorage::READ); storage21["f"] >> _features[21]; storage21.release();
    //    cv::FileStorage storage22("f_22.xml", cv::FileStorage::READ); storage22["f"] >> _features[22]; storage22.release();
    //    cv::FileStorage storage23("f_23.xml", cv::FileStorage::READ); storage23["f"] >> _features[23]; storage23.release();

    //cv::FileStorage storagexx("xx.yml", cv::FileStorage::READ); storagexx.open("xx.yml", cv::FileStorage::READ); storagexx["xx"] >> test; storagexx.release();
    //cv::FileStorage storagexx("xx.xml", cv::FileStorage::READ); storagexx["xx"] >> txx; storagexx.release();
    //cv::FileStorage storagexy("xy.xml", cv::FileStorage::READ); storagexy["xy"] >> txy; storagexy.release();
    //cv::FileStorage storagexz("xz.xml", cv::FileStorage::READ); storagexz["xz"] >> txz; storagexz.release();
    //cv::FileStorage storageyy("yy.xml", cv::FileStorage::READ); storageyy["yy"] >> tyy; storageyy.release();
    //cv::FileStorage storageyz("yz.xml", cv::FileStorage::READ); storageyz["yz"] >> tyz; storageyz.release();
    //cv::FileStorage storagezz("zz.xml", cv::FileStorage::READ); storagezz["zz"] >> tzz; storagezz.release();

    //std::cout << test.rows << ":" << test.cols << std::endl;
    for(int i = 0; i < 18; i++)
    {
        std::cout << _features[i].rows << ":" << _features[i].cols << std::endl;
    }
    
    _cols = _features[0].cols;
    _rows = _features[0].rows;

}

void MainWindow::initTargetPts()
{
    cv::Mat temp;
    //populate desired corner mask
    cv::FileStorage storagemsk("corner_mask.xml", cv::FileStorage::READ); storagemsk["f"] >> temp; storagemsk.release();
    //_desired_cormsk = cv::imread("corner_mask.yml");
    _matTargetPts = temp.clone();
    _iNumTargetPts = countPts(_matTargetPts);
    std::cout << "Target: rows: " << _matTargetPts.rows << "cols: " << _matTargetPts.cols << std::endl;
    std::cout << "Total Feature Points: "<< _iNumTargetPts << std::endl;
}

void MainWindow::initPlotingViews()
{
    // plot for generations
    ui->plot->addGraph();
    ui->plot->graph(0)->setLineStyle(QCPGraph::lsNone);
    ui->plot->graph(0)->setScatterStyle(QCP::ScatterStyle(QCP::ScatterStyle::ssCircle));
    ui->plot->graph(0)->setPen(QPen(Qt::blue));
    ui->plot->graph(0)->setName("Avg");
    ui->plot->addGraph();
    ui->plot->graph(1)->setPen(QPen(Qt::red));
    ui->plot->graph(1)->setLineStyle(QCPGraph::lsNone);
    ui->plot->graph(1)->setScatterStyle(QCP::ScatterStyle(QCP::ScatterStyle::ssCross));;
    ui->plot->graph(1)->setName("Std");
    ui->plot->addGraph();
    ui->plot->graph(2)->setPen(QPen(Qt::green));
    ui->plot->graph(2)->setLineStyle(QCPGraph::lsNone);
    ui->plot->graph(2)->setScatterStyle(QCP::ScatterStyle(QCP::ScatterStyle::ssTriangle));;
    ui->plot->graph(2)->setName("Max");
    ui->plot->xAxis->setRange(0, 200);
    ui->plot->yAxis->setRange(0, 1.0);
    ui->plot->xAxis->setLabel("Generation");
    ui->plot->yAxis->setLabel("Fitness Value");
    ui->plot->setRangeDrag(Qt::Horizontal);
    ui->plot->setRangeZoom(Qt::Horizontal);
    ui->plot->legend->setVisible(true);
    QFont legendFont = font();  // start out with MainWindow's font..
    legendFont.setPointSize(9); // and make a bit smaller for legend
    ui->plot->legend->setFont(legendFont);
    ui->plot->legend->setPositionStyle(QCPLegend::psRight);
    ui->plot->legend->setBrush(QBrush(QColor(255,255,255,230)));

    // plot for individual statistics
    ui->plot_individual->addGraph();
    ui->plot_individual->graph(0)->setLineStyle(QCPGraph::lsNone);
    ui->plot_individual->graph(0)->setScatterStyle(QCP::ScatterStyle(QCP::ScatterStyle::ssCircle));
    ui->plot_individual->graph(0)->setPen(QPen(Qt::blue));
    ui->plot_individual->graph(0)->setName("Recall");
    ui->plot_individual->addGraph();
    ui->plot_individual->graph(1)->setPen(QPen(Qt::red));
    ui->plot_individual->graph(1)->setLineStyle(QCPGraph::lsNone);
    ui->plot_individual->graph(1)->setScatterStyle(QCP::ScatterStyle(QCP::ScatterStyle::ssCross));;
    ui->plot_individual->graph(1)->setName("Precision");
    ui->plot_individual->addGraph();
    ui->plot_individual->graph(2)->setPen(QPen(Qt::green));
    ui->plot_individual->graph(2)->setLineStyle(QCPGraph::lsNone);
    ui->plot_individual->graph(2)->setScatterStyle(QCP::ScatterStyle(QCP::ScatterStyle::ssTriangle));;
    ui->plot_individual->graph(2)->setName("Fitness");
    ui->plot_individual->xAxis->setRange(-16, 16);
    ui->plot_individual->yAxis->setRange(0, 1.0);
    ui->plot_individual->xAxis->setLabel("Threshold");
    ui->plot_individual->yAxis->setLabel("Value");
    ui->plot_individual->setRangeDrag(Qt::Horizontal);
    ui->plot_individual->setRangeZoom(Qt::Horizontal);
    ui->plot_individual->legend->setVisible(true);
    ui->plot_individual->legend->setFont(legendFont);
    ui->plot_individual->legend->setPositionStyle(QCPLegend::psLeft);
    ui->plot_individual->legend->setBrush(QBrush(QColor(255,255,255,230)));
}


cv::Mat MainWindow::preOrder(QDomElement node, bool testing)
{
    cv::Mat result;
    // operators
    if ( node.tagName() == ("STM2"))
    {
        qDebug() << "STM2***";
        qDebug() << node.firstChildElement().tagName();
        if ( node.firstChildElement().text() == ("-") )
        {
            qDebug() << "MatSub";
            cv::Mat arg1 = preOrder(node.firstChildElement().nextSiblingElement());
            cv::Mat arg2 = preOrder(node.firstChildElement().nextSiblingElement().nextSiblingElement());
            result = arg1 - arg2;
            return result;
        }
        if ( node.firstChildElement().text() == ("+") )
        {
            qDebug() << "MatAdd";
            cv::Mat arg1 = preOrder(node.firstChildElement().nextSiblingElement());
            cv::Mat arg2 = preOrder(node.firstChildElement().nextSiblingElement().nextSiblingElement());
            result = arg1 + arg2;
            return result;
        }
        if ( node.firstChildElement().text() == ("*") )
        {
            qDebug() << "MatMul";
            cv::Mat arg1 = preOrder(node.firstChildElement().nextSiblingElement());
            cv::Mat arg2 = preOrder(node.firstChildElement().nextSiblingElement().nextSiblingElement());
            result =  arg1.mul(arg2);
            return result;
        }
    }
    if (node.tagName() == ("STM"))
    {
        qDebug() << "STM";
        result = preOrder(node.firstChildElement());
        return result;
    }
    if (node.tagName() == ("CONST"))
    {
        qDebug() << "CONST";
        float cst = node.text().toFloat();//double cst = node.
        cst = cst / 100;
        cv::Mat_<float> cstMat(_rows, _cols, cst);
        assert(_cols == cstMat.cols);
        return cstMat;
    }
    if (node.tagName() == ("VAR"))
    {
        QString strVar = node.text();
        //qDebug() << "******" << strVar;
        // operants
        qDebug() << "--hh--" << strVar.mid(2);
        int f_index = strVar.mid(2).toInt();
        return _features[f_index];
        //            if (  strVar == ("v_0") )
        //            {
        //                return txx;
        //            }
        //            if (  strVar == ("v_1") )
        //            {
        //                return txy;
        //            }
        //            if (  strVar == ("v_2") )
        //            {
        //                return txz;
        //            }
        //            if (  strVar == ("v_3") )
        //            {
        //                return tyy;
        //            }
        //            if (  strVar == ("v_4") )
        //            {
        //                return tyz;
        //            }
        //            if (  strVar == ("v_5") )
        //            {
        //                return tzz;
        //            }
    }
    //******************Beagle XML******************

    // operants
    if ( node.tagName() == ("xx") )
    {
        return _features[12];
    }
    if ( node.tagName() == ("xy") )
    {
        return _features[13];
    }
    if ( node.tagName() == ("xz") )
    {
        return _features[14];
    }
    if ( node.tagName() == ("yy") )
    {
        return _features[15];
    }
    if ( node.tagName() == ("yz") )
    {
        return _features[16];
    }
    if ( node.tagName() == ("zz") )
    {
        return _features[17];
    }
    // operators
    if ( node.tagName() == ("MatSub") )
    {
        qDebug() << "MatSub";
        cv::Mat arg1 = preOrder(node.firstChildElement());
        cv::Mat arg2 = preOrder(node.firstChildElement().nextSiblingElement());
        result = arg1 - arg2;
        return result;
    }
    if ( node.tagName() == ("MatAdd") )
    {
        qDebug() << "MatAdd";
        cv::Mat arg1 = preOrder(node.firstChildElement() );
        cv::Mat arg2 = preOrder(node.firstChildElement().nextSiblingElement());
        result = arg1 + arg2;
        return result;
    }
    if ( node.tagName() == ("MatMul") )
    {
        qDebug() << "MatMul";
        cv::Mat arg1 = preOrder(node.firstChildElement() );
        cv::Mat arg2 = preOrder(node.firstChildElement().nextSiblingElement());
        result =  arg1.mul(arg2);
        return result;
    }
    if ( node.tagName() == ("MatMulDou") )
    {
        qDebug() << "MatMulDou";
        cv::Mat arg1 = preOrder(node.firstChildElement() );
        double d = node.firstChildElement().nextSiblingElement().attribute("value").toDouble();
        result =  arg1*d;
        return result;
    }
    if ( node.tagName() == ("MatSquare") )
    {
        qDebug() << "MatSquare";
        cv::Mat arg1 = preOrder(node.firstChildElement());
        result = arg1.mul(arg1);
        return result;
    }
    qDebug() << "Error";
    qDebug() << "..................";
}

//void on_pushButton_evaluate_statistics_clicked()
//{
//    // populize domdocument
//    QDomDocument doc;
//    if (!doc.setContent(ui->textEdit->toPlainText())) {
//        qDebug() << "Fail to populize domdocument...";
//        return;
//    }

//    cv::Mat result, norm;

//    // get feature point detection result
//    result = preOrder( doc.firstChildElement() );
//    if (_bShow) {
//        cv::imshow("result", result);
//    }

//    // normalizing to period [0, 255]
//    cv::normalize( result, norm, 0, 255, cv::NORM_MINMAX, CV_8UC1, cv::Mat() );
//    cv::imshow("Response", norm);
////    cv::resizeWindow("Response", 256, 256);

//    // clear vectors
//    _vcPrecision.clear();
//    _vcRecall.clear();
//    _vcFitness.clear();
//    _vcThreshold.clear();

//    double lFitness = 0;
//    for (int c = 0; c < 32; c++)
//    {
//        // set a detection threshold
//        int threshold = 8 * c;
//        _vcThreshold.append(threshold);

//        // calculate detected pts
//        cv::Mat matDetectedPts;
//        FeatureEvalOp::detectFeaturePts(norm, matDetectedPts, threshold);

//        // calculate recall and precision
//        double recall, precision, fitness;
//        FeatureEvalOp::calculateStatistics(matDetectedPts, _matTargetPts, _iNumTargetPts, recall, precision, fitness);

//        _vcPrecision.push_back(precision);
//        _vcRecall.push_back(recall);
//        _vcFitness.append(fitness);

//        if (lFitness < fitness) lFitness = fitness;
//    }

//    ui->plot_individual->graph(0)->setData(_vcThreshold, _vcRecall);
//    ui->plot_individual->graph(1)->setData(_vcThreshold, _vcPrecision);
//    ui->plot_individual->graph(2)->setData(_vcThreshold, _vcFitness);
//    ui->plot_individual->replot();
//    ui->statusbar->showMessage("Fitness value: " + QString::number(lFitness));
//}

void MainWindow::on_pushButton_clicked()
{
    // populize domdocument
    QDomDocument doc;
    if (!doc.setContent(ui->textEdit_2->toPlainText()))
    {
        qDebug() << "Fail to populize domdocument...";
        return ;
    }

    cv::Mat result, norm;
    qDebug() << doc.firstChildElement().tagName();
    qDebug() << doc.firstChildElement().firstChildElement().tagName();
    // get feature point detection result
    result = preOrder( doc.firstChildElement().firstChildElement() );

    if (false)
    {
        cv::imshow("result", result);
    }

    // normalizing to period [0, 255]
    cv::normalize( result, norm, -16, 16, cv::NORM_MINMAX, CV_16SC1, cv::Mat() );
    cv::Mat response;
    cv::normalize( result, response, 0, 255, cv::NORM_MINMAX, CV_8UC1, cv::Mat() );
    cv::imshow("Response", response);
    //cv::resizeWindow("Response", 256, 256);

    // clear vectors
    _vcPrecision.clear();
    _vcRecall.clear();
    _vcFitness.clear();
    _vcThreshold.clear();

    double lFitness = 0;
    int max_threshold = 0;

    // value interval [-16, 16]; threshold interval [-15, 15]
    for (int c = 0; c < 31; c++)
    {
        // set a detection threshold
        int threshold = c - 15;
        _vcThreshold.append(threshold);

        // calculate detected pts
        cv::Mat matDetectedPts;
        detectFeaturePts(norm, matDetectedPts, threshold);

        // calculate recall and precision
        double recall, precision, fitness;
        calculateStatistics(matDetectedPts, _matTargetPts, _iNumTargetPts, recall, precision, fitness);

        _vcPrecision.append(precision);
        _vcRecall.append(recall);
        _vcFitness.append(fitness);

        if (lFitness < fitness)
        {
            lFitness = fitness;
            max_threshold = c;
        }
    }

    ui->plot_individual->graph(0)->setData(_vcThreshold, _vcRecall);
    ui->plot_individual->graph(1)->setData(_vcThreshold, _vcPrecision);
    ui->plot_individual->graph(2)->setData(_vcThreshold, _vcFitness);
    ui->plot_individual->replot();
    int fitness = (int)(lFitness*_iNorDist[max_threshold]*10000);
    if (fitness == 0) fitness = 1;
    ui->statusbar->showMessage("Fitness value: " + QString::number(fitness));
}

void MainWindow::on_actionStart_triggered()
{
    static bool isStart = false;
    if (!isStart)
    {
        isStart = true;
        initSocket();
    }
}
