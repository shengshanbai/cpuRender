#pragma once
#include "ObjModel.h"
#include <opencv2/opencv.hpp>

class RenderContext
{
public:
	RenderContext(int width,int height);
	~RenderContext();
	void clearDepthBuffer();
	void drawModel(ObjModel& model,cv::Mat4f& transform);
private:
	int width;
	int height;
	cv::Mat framebuffer;
	cv::Mat depthbuffer;
	cv::Mat4f viewport;
};