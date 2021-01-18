#pragma once
#include "ObjModel.h"
#include <opencv2/opencv.hpp>

class RenderContext
{
public:
	RenderContext(int width,int height);
	~RenderContext();
	void clearDepthBuffer();
	void drawModel(ObjModel& model,cv::Mat& transform);
	void writeTo(std::string filename);
private:
	void blendAlpha();
	struct DepthColor
	{
		cv::Vec4b color;
		float depth;
	};
	int width;
	int height;
	cv::Mat framebuffer;
	cv::Mat depthbuffer;
	cv::Mat4f viewport;
	std::unordered_map<long, std::vector<DepthColor>> blendmap;
};