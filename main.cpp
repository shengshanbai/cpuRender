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
void onMouse(int event,int x, int y,int flags,void* param) {

}

void mainLoop() {
	string windowName = "main";
	int windowWidth = 800;
	int windowHight = 800;
    ObjModel model;
    model.loadObj("../data/toukui/toukui.obj","../data/toukui");
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
void mainLoop() {
	string windowName = "main";
	int windowWidth = 864;
	int windowHight = 864;
  ObjModel model;
  model.loadObj("../data/toukui/toukui.obj","../data/toukui");
	cv::Mat image(windowHight, windowWidth, CV_8UC4);
}
#endif

int main() {
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