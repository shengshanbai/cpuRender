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
	bool loadObj(std::string model, std::string mtl_dir);
	std::unique_ptr<cv::Vec4f[], free_delete> tranVertices(cv::Mat &tMat);
	std::unique_ptr<cv::Vec4f[], free_delete> tranNormals(cv::Mat &tMat);
	cv::Vec4f &getVertColor(int vid);
	cv::Mat &getMaterial(int id);
	int getNumFaces();
	cv::Vec4f &getVertex(int faceId, int subId);
	cv::Vec2f &getUV(int tid);
	std::vector<tinyobj::shape_t> &getShapes();

private:
	void copyV3fTo4f(float *src, std::unique_ptr<cv::Vec4f[], free_delete> &dst, int vCount);
	void copyV2fTo2f(float *src, std::unique_ptr<cv::Vec2f[], free_delete> &dst, int vCount);
	void copyIntToAssign(int *src, std::unique_ptr<int[], free_delete> &dst, int dStart, int count);
	std::unique_ptr<cv::Vec4f[], free_delete> mat4MultiVec4f(std::unique_ptr<cv::Vec4f[], free_delete> &src, cv::Mat &tMat, int count);

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
