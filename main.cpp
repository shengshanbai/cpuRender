#include "ObjModel.h"
#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "render.h"

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

using namespace std;

#ifdef _WIN32
void onMouse(int event, int x, int y, int flags, void *param)
{
}

void mainLoop()
{
	string windowName = "main";
	int windowWidth = 800;
	int windowHight = 800;
	ObjModel model;
	model.loadObj("../data/toukui/toukui.obj", "../data/toukui");
	cv::namedWindow(windowName, cv::WINDOW_NORMAL);
	cv::resizeWindow(windowName, windowWidth, windowHight);
	cv::setMouseCallback(windowName, onMouse, 0);
	cv::Mat image(windowHight, windowWidth, CV_8UC4);
	RenderContext rcontext(windowWidth, windowHight);
	while (cv::getWindowProperty(windowName, cv::WND_PROP_VISIBLE) == 1)
	{
		cv::imshow(windowName, image);
		int key_code = cv::waitKey(20);
	}
	cv::destroyWindow(windowName);
}
#else
void mainLoop()
{
	string windowName = "main";
	int windowWidth = 864;
	int windowHight = 864;
	ObjModel model;
	model.loadObj("../data/santa_hat/maozi_3.obj", "../data/santa_hat");
	ObjModel headModel;
	headModel.loadObj("../data/smooth_head.obj", "../data");
	RenderContext rContex(windowWidth, windowHight);
	cv::Mat transMat(4, 4, CV_32FC1);
	transMat.at<float>(0, 0) = 3.127707204839680344e-04;
	transMat.at<float>(0, 1) = 2.411879759165458381e-04;
	transMat.at<float>(0, 2) = -9.514510650963832212e-04;
	transMat.at<float>(0, 3) = 2.644506772359212050e+02;
	transMat.at<float>(1, 0) = -2.443285041711836252e-04;
	transMat.at<float>(1, 1) = -9.311167717290420604e-04;
	transMat.at<float>(1, 2) = -3.046750680368859321e-04;
	transMat.at<float>(1, 3) = 1.784243663152058730e+02;
	transMat.at<float>(2, 0) = 9.524971537757664919e-04;
	transMat.at<float>(2, 1) = -3.339596842124592513e-04;
	transMat.at<float>(2, 2) = 2.206901068954418013e-04;
	transMat.at<float>(2, 3) = -1.138961051305135044e+02;
	transMat.at<float>(3, 0) = 0;
	transMat.at<float>(3, 1) = 0;
	transMat.at<float>(3, 2) = 0;
	transMat.at<float>(3, 3) = 1;
	cv::Mat modelT = transMat.clone();
	modelT.col(0) *= 1000;
	modelT.col(1) *= 1000;
	modelT.col(2) *= 1000;
	auto start = chrono::system_clock::now();
	for (int i = 0; i < 10; i++)
	{
		rContex.drawModel(headModel, transMat);
		rContex.drawModel(model, transMat);
		rContex.blendAlpha();
		rContex.clearDepthBuffer();
	}
	auto end = chrono::system_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
	cout << "花费了"
		 << double(duration.count()) * chrono::microseconds::period::num / chrono::microseconds::period::den
		 << "秒" << endl;
	rContex.writeTo("./output.png");
}
#endif

int main()
{
	mainLoop();
	/*
    ObjModel objModel;
    auto start = chrono::system_clock::now();
    objModel.loadObj("../data/toukui/toukui.obj","../data/toukui");
    auto end = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout <<  "花费了" 
     << double(duration.count()) * chrono::microseconds::period::num / chrono::microseconds::period::den 
     << "秒" << endl;
    MaskedOcclusionCulling *moc=MaskedOcclusionCulling::Create();
    moc->SetResolution(864,480);
    moc->ClearBuffer();
    MaskedOcclusionCulling::Destroy(moc);
    */
#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}