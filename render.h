#pragma once
#include "ObjModel.h"
#include <opencv2/opencv.hpp>

#define _MM_ROUND_MASK        0x6000
#define _MM_ROUND_NEAREST     0x0000
#define _MM_ROUND_DOWN        0x2000
#define _MM_ROUND_UP          0x4000
#define _MM_ROUND_TOWARD_ZERO 0x6000

#define _MM_SET_ROUNDING_MODE(mode)                                 \
            _mm_setcsr((_mm_getcsr() & ~_MM_ROUND_MASK) | (mode))
			
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
	void renderTriangle(cv::Vec4f (&vs)[3],cv::Vec4f (&vns)[3],cv::Vec2f (&uvs)[3],cv::Mat& texture);

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