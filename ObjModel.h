#pragma once
#include "tiny_obj_loader.h"
#include "utils.h"
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
	int getNumFaces();
	cv::Vec4f& getVertex(int faceId,int subId);
	cv::Vec2f& getUV(int faceId,int subId);
private:
	void copyVertices(float* src, std::unique_ptr<cv::Vec4f[], free_delete>& dst,int vCount);

	std::unique_ptr<cv::Vec4f[], free_delete> vertices;
	std::unique_ptr<cv::Vec2f[], free_delete> uvs;
	std::unique_ptr<cv::Vec4f[], free_delete> normals;
	std::unique_ptr<cv::Vec4f[], free_delete> verticeColors;
	std::unique_ptr<int[], free_delete> materialIds;
	std::unique_ptr<cv::Vec3i[], free_delete> faces;
	int num_faces;
	int materalCount;
	std::vector<cv::Mat> diffuseTextures;
};

