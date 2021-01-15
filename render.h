#pragma once
#include "ObjModel.h"
#include <opencv2/opencv.hpp>

void drawModel(ObjModel& model,cv::Mat& image);
void fill_triangle(cv::Mat& image,cv::Vec3f& point0,cv::Vec3f& point1,cv::Vec3f& point2,
    cv::Vec2f& uv0,cv::Vec2f& uv1,cv::Vec2f& uv2,cv::Mat& zbuffer,cv::Mat& texture,float intensity);