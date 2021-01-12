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
    auto pRtMat=make_aligned_array<float>(16,sizeof(float)*12);
    pRtMat[0]=3.127707204839680344e-04;
    pRtMat[1]=2.411879759165458381e-04;
    pRtMat[2]=-9.514510650963832212e-04;
    pRtMat[3]=2.644506772359212050e+02;
    pRtMat[4]=-2.443285041711836252e-04;
    pRtMat[5]=-9.311167717290420604e-04;
    pRtMat[6]=-3.046750680368859321e-04;
    pRtMat[7]=1.784243663152058730e+02;
    pRtMat[8]=9.524971537757664919e-04;
    pRtMat[9]=-3.339596842124592513e-04;
    pRtMat[10]=2.206901068954418013e-04;
    pRtMat[11]=-1.138961051305135044e+02;
    ObjModel objModel;
    auto start = chrono::system_clock::now();
    objModel.loadObj("../data/toukui/toukui.obj","../data/toukui");
    for(int i=0;i<20;i++){
        auto pOut=objModel.transform(R,offset);
        cout<<"data:"<<pOut.at<float>(0,0)<<","<<pOut.at<float>(0,1)<<","<<pOut.at<float>(0,2)<<endl;
        //data:264.547,178.389,-113.877
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