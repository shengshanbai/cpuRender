#pragma once
#include "ObjModel.h"
#include <opencv2/opencv.hpp>

void drawModel(ObjModel& model,cv::Mat& image);
void fill_triangle(cv::Mat& image,cv::Vec2i& point0,cv::Vec2i& point1,cv::Vec2i& point3,cv::Vec4b& color);