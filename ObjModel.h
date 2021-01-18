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
	std::unique_ptr<cv::Vec4f[],free_delete> tranVertices(cv::Mat& tMat);
	cv::Vec4b getMixColor(std::vector<int>& ids, cv::Vec3f& bc_screen);
	cv::Vec4b getTextureColor(int materialId, std::vector<int>& tids, cv::Vec3f& bc_screen);
	int getNumFaces();
	cv::Vec4f& getVertex(int faceId,int subId);
	cv::Vec2f& getUV(int faceId,int subId);
	std::vector<tinyobj::shape_t>& getShapes();
private:
	void copyV3fTo4f(float* src, std::unique_ptr<cv::Vec4f[], free_delete>& dst,int vCount);
	void copyV2fTo2f(float* src,std::unique_ptr<cv::Vec2f[], free_delete>& dst,int vCount);
	void copyIntToAssign(int* src,std::unique_ptr<int[], free_delete>& dst,int dStart,int count);

	std::unique_ptr<cv::Vec4f[], free_delete> vertices;
	std::unique_ptr<cv::Vec2f[], free_delete> uvs;
	std::unique_ptr<cv::Vec4f[], free_delete> normals;
	std::unique_ptr<cv::Vec4f[], free_delete> verticeColors;
	std::vector<tinyobj::shape_t> shapes;
	int num_faces;
	int materalCount;
	int vertCount;
	std::vector<cv::Mat> diffuseTextures;
};

