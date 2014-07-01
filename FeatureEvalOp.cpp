
/*!
 *  \file   FeatureEvalOp.cpp
 *  \brief  Implementation of the class FeatureEvalOp.
 *  \author Li Yuejiang
 */

#include <beagle/GP.hpp>
#include "FeatureEvalOp.hpp"

#include <cmath>
#include <fstream>
#include <algorithm>

using namespace Beagle;

int FeatureEvalOp::countPts(const cv::Mat & img)
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
 *  \brief Construct a new feature detector evaluation operator.
 *  \param inFilename Default filename used.
 */
FeatureEvalOp::FeatureEvalOp(Beagle::string inFilename) :
  GP::EvaluationOp("FeatureEvalOp"),
  mFilename(NULL),
  mFilenameDefault(inFilename)
{ }


/*!
 *  \brief Initialize the feature detector evaluation operator.
 *  \param ioSystem System of the evolution.
 */
void FeatureEvalOp::initialize(Beagle::System& ioSystem)
{
  Beagle::GP::EvaluationOp::initialize(ioSystem);

  if(ioSystem.getRegister().isRegistered("TrainingImage.filename")) {
    mFilename = castHandleT<String>(ioSystem.getRegister()["TrainingImage.filename"]);
  } else {
    mFilename = new String(mFilenameDefault);
    Register::Description lDescription(
      "Filename of Training image",
      "String",
      mFilenameDefault,
      "Name of the file containing the Training Image."
    );
    ioSystem.getRegister().addEntry("TrainingImage.filename", mFilename, lDescription);
  }
}


/*!
 *  \brief Evaluate the individual fitness for the feature detection problem.
 *  \param inIndividual Individual to evaluate.
 *  \param ioContext Evolutionary context.
 *  \return Handle to the fitness measure,
 */
Fitness::Handle FeatureEvalOp::evaluate(GP::Individual& inIndividual, GP::Context& ioContext)
{
	double lFitness = 0;

    setValue("AIx2", AIx2, ioContext);
	setValue("AIy2", AIy2, ioContext);
	setValue("AIxIy", AIxIy, ioContext);
	setValue("Ix", Ix, ioContext);
	setValue("Iy", Iy, ioContext);

    Mat lResult;
	std::cout << inIndividual.serialize() << std::endl;
    inIndividual.run(lResult, ioContext);

	cv::Mat_<float>& result = lResult.getWrappedValue();
    cv::Mat norm;
    // normalizing to period [0, 255]
    cv::normalize( result, norm, 0, 255, cv::NORM_MINMAX, CV_8UC1, cv::Mat() );
    cv::imshow("Response", norm);
    cv::waitKey(1);

	std::vector<double> vprecision, vrecall;

    for (int c = 0; c < 32; c++)
    {

        // set a detection threshold
        int threshold = 8 * c;

        // calculate detected pts
        cv::Mat matDetectedPts;
        FeatureEvalOp::detectFeaturePts(norm, matDetectedPts, threshold);

        // calculate recall and precision
        double recall, precision, fitness;
        FeatureEvalOp::calculateStatistics(matDetectedPts, _desired_cormsk, num_desired, recall, precision, fitness);

        vprecision.push_back(precision);
        vrecall.push_back(recall);
		if (lFitness < fitness) lFitness = fitness;
    }

	std::cout << "Fitness value:  " << lFitness << std::endl;
	return new FitnessSimple(lFitness);
}


/*!
 *  \brief Post-initialization hook the spambase evaluation operator.
 *  \param ioSystem System of the evolution.
 */
void FeatureEvalOp::postInit(Beagle::System& ioSystem)
{
	GP::EvaluationOp::postInit(ioSystem);

	// read training image
	if((mFilename!=NULL) && (mFilename->getWrappedValue().empty()==false)) {
		Beagle_LogBasicM(
		ioSystem.getLogger(),
		"evaluation", "FeatureEvalOp",
		Beagle::string("Reading spambase data from file \"")+mFilename->getWrappedValue()+"\""
    );
	
	mTrainingImg = cv::imread("Training.png");
	cv::Mat gray;
	if(mTrainingImg.channels() == 1)
	{
		gray = mTrainingImg.clone();
		cv::cvtColor(mTrainingImg, mTrainingImg, CV_GRAY2BGR);
	} else
	{
		cv::cvtColor(mTrainingImg, gray, CV_BGR2GRAY);
	}
	gray.convertTo(I.getWrappedValue(), CV_32F);
	cv::Sobel( gray, Ix.getWrappedValue(), CV_32F, 1, 0, 3, 8, 0, cv::BORDER_DEFAULT );
	cv::Sobel( gray, Iy.getWrappedValue(), CV_32F, 0, 1, 3, 8, 0, cv::BORDER_DEFAULT );

	Ix2.getWrappedValue() = Ix.getWrappedValue().mul(Ix.getWrappedValue());
	Iy2.getWrappedValue() = Iy.getWrappedValue().mul(Iy.getWrappedValue());
	IxIy.getWrappedValue() = Ix.getWrappedValue().mul(Iy.getWrappedValue());

	cv::boxFilter(Ix2.getWrappedValue(), AIx2.getWrappedValue(), Ix2.getWrappedValue().depth(), cv::Size(2, 2), cv::Point(-1,-1), false, cv::BORDER_DEFAULT );
	cv::boxFilter(Iy2.getWrappedValue(), AIy2.getWrappedValue(), Iy2.getWrappedValue().depth(), cv::Size(2, 2), cv::Point(-1,-1), false, cv::BORDER_DEFAULT );
	cv::boxFilter(IxIy.getWrappedValue(), AIxIy.getWrappedValue(), IxIy.getWrappedValue().depth(), cv::Size(2, 2), cv::Point(-1,-1), false, cv::BORDER_DEFAULT );


	// read desired feature postion
	//char *inname = Beagle::string(".jpg") + mFilename->getWrappedValue()+Beagle::string(".jpg");
    std::ifstream infile(mFilename->getWrappedValue()+Beagle::string(".txt"));

    if (!infile) {
		std::cout << "There was a problem opening file "
					<< mFilename->getWrappedValue()
					<< " feature point positions."
					<< std::endl;
        return;
    }

    std::cout << "Feature position file Opened " << mFilename->getWrappedValue() << " for reading." << std::endl;
	cv::Mat src = mTrainingImg.clone();
    _desired_cormsk = cv::Mat::zeros(src.size(), CV_8UC1);
    int x, y;
    num_desired = 0;
    while (infile >> x && infile >> y) {
        x /= 1000;
        y /= 1000;
        // Drawing a circle around corners
        cv::circle( src, cv::Point( x, y ), 1,  cv::Scalar(255, 255, 0), 1, 8, 0 );
        _desired_cormsk.at<uchar>( y, x ) = 255;
        cv::circle( _desired_cormsk, cv::Point( x, y ), 1,  cv::Scalar(255), 1, 8, 0 );
        num_desired++;
    }
    cv::imshow("Desired corner mask", _desired_cormsk);
    cv::imshow("Desired corner position", src);
	cv::waitKey(1);

  }
}


/*!
 *  \brief Read data of the spambase problem.
 *  \param inFilename Name of the file in which the spambase problem data are.
 *  \param inSizeData Number of entry in the data base.
 *  \throw InternalException When the file format is not valid.
 */
void FeatureEvalOp::readData(Beagle::string inFilename, unsigned int inSizeData)
{
}


/*!
 *  \brief extract feature points from reponse image.
 *  \param result in interest points reponse mat
 *  \param ptsMask out a mask image with 255 indicate detected feature points
 */
void FeatureEvalOp::detectFeaturePts(const cv::Mat & result, cv::Mat & ptsMask, int threshold)
{
    ptsMask = cv::Mat::zeros(result.size(), CV_8UC1);

    // threshold to obtain feature pts
    for( int j = 0; j < result.rows ; j++ )
    {
        for( int i = 0; i < result.cols; i++ )
        {
            if( (int) result.at<uchar>(j,i) > threshold )
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

void FeatureEvalOp::calculateStatistics(const cv::Mat &detected_mask, const cv::Mat &desired_mask, int desiredTotalNumPts, double &recall, double &precision, double &fitness, bool isShow)
{
    int num_matched, num_response;

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
