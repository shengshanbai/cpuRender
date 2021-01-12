#pragma once
#include <tiny_obj_loader.h>
#include <string>
#include <opencv2/opencv.hpp>
#include <vector>

class ObjModel
{
public:
	ObjModel();
	bool loadObj(std::string model,std::string mtl_dir);
	cv::Mat transform(cv::Mat& R,cv::Mat& offset);
	cv::Vec4b getMixColor(std::vector<int>& ids, cv::Vec3f& bc_screen);
	cv::Vec4b getTextureColor(int materialId, std::vector<int>& tids, cv::Vec3f& bc_screen);
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::vector<cv::Mat> textures;
	cv::Mat worldVertices;
};

