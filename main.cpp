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

cv::Vec4b white = {255, 255, 255,255};
cv::Vec4b blue = {255, 0, 0,255};
cv::Vec4b green = {0, 255, 0,255};
cv::Vec4b red = {0, 0, 255,255};

vec2i_t t0[3] = {{10, 70}, {50, 160}, {70, 80}};
vec2i_t t1[3] = {{180, 50}, {150, 1}, {70, 180}};
vec2i_t t2[3] = {{180, 150}, {120, 160}, {130, 180}};

void onMouse(int event,int x, int y,int flags,void* param) {

}

void mainLoop() {
	string windowName = "main";
	int windowWidth = 800;
	int windowHight = 800;
    ObjModel model;
    model.loadObj("../data/african_head.obj","./data");
	cv::namedWindow(windowName, cv::WINDOW_NORMAL);
	cv::resizeWindow(windowName, cv::Size(windowWidth, windowHight));
	cv::setMouseCallback(windowName, onMouse, 0);
	cv::Mat image(windowHight, windowWidth, CV_8UC4);
    fill_triangle(image, t0[0], t0[1], t0[2], blue);
    fill_triangle(image, t1[0], t1[1], t1[2], green);
    fill_triangle(image, t2[0], t2[1], t2[2], red);
	while (cv::getWindowProperty(windowName, cv::WND_PROP_VISIBLE) == 1)
	{
		cv::imshow(windowName, image);
		int key_code = cv::waitKey(20);
	}
	cv::destroyWindow(windowName);
}

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