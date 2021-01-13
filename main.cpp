#include "MaskedOcclutionCulling.h"
#include "ObjModel.h"
#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>

using namespace std;


int main() {
    string windowName="main";
    int windowWidth=864;
    int windowHight=480;
    cv::namedWindow(windowName,cv::WINDOW_NORMAL);
    cv::resizeWindow(windowName,cv::Size(windowWidth,windowHight));
    create_aligned_mat(32,windowHight,windowWidth,CV_8UC4);
    while (cv::getWindowProperty(windowName,cv::WND_PROP_VISIBLE)==1)
    {
        /* code */
    }
    cv::destroyWindow(windowName);
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
    return 0;
}