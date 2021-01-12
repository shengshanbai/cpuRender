#include "MaskedOcclutionCulling.h"
#include "ObjModel.h"
#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>

using namespace std;

int main() {
    cv::Mat R=(cv::Mat_<float>(3, 3) << 3.127707204839680344e-04,2.411879759165458381e-04,-9.514510650963832212e-04,
		-2.443285041711836252e-04,-9.311167717290420604e-04,-3.046750680368859321e-04,
		9.524971537757664919e-04, -3.339596842124592513e-04,2.206901068954418013e-04);
	cv::Mat offset = (cv::Mat_<float>(3, 1) << 2.644506772359212050e+02,1.784243663152058730e+02,-1.138961051305135044e+02);
    ObjModel objModel;
    auto start = chrono::system_clock::now();
    for(int i=0;i<10;i++){
        objModel.loadObj("../data/toukui/toukui.obj","../data/toukui");
        objModel.transform(R,offset);
    }
    auto end = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout <<  "花费了" 
     << double(duration.count()) * chrono::microseconds::period::num / chrono::microseconds::period::den 
     << "秒" << endl;
    MaskedOcclusionCulling *moc=MaskedOcclusionCulling::Create();
    moc->SetResolution(864,480);
    moc->ClearBuffer();
    MaskedOcclusionCulling::Destroy(moc);
    return 0;
}