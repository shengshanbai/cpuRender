#pragma once
#include "ObjModel.h"
#include <opencv2/opencv.hpp>

void drawModel(ObjModel& model,cv::Mat& image);
void fill_triangle(cv::Mat& image,vec2i_t& point0,vec2i_t& point1,vec2i_t& point3,cv::Vec4b& color);