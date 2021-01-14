#pragma once
#include "tiny_obj_loader.h"
#include "utils.h"
#include "geometry.h"
#include <string>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>

class ObjModel
{
public:
	ObjModel();
	bool loadObj(std::string model,std::string mtl_dir);
	std::unique_ptr<float[],free_delete> transform(std::unique_ptr<float[],free_delete>& rtMat);
	cv::Vec4b getMixColor(std::vector<int>& ids, cv::Vec3f& bc_screen);
	cv::Vec4b getTextureColor(int materialId, std::vector<int>& tids, cv::Vec3f& bc_screen);
	std::vector<cv::Mat> textures;
private:
	std::unique_ptr<vec3f_t[],free_delete> vertices;
	std::unique_ptr<vec2f_t[],free_delete> uvs;
	std::unique_ptr<vec3f_t[],free_delete> normals;
	int num_faces;
};

