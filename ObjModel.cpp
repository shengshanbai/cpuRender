#include "ObjModel.h"
#include <iostream>
#include <cstdlib>
#include "log.h"

using namespace std;

ObjModel::ObjModel()
{
}

bool ObjModel::loadObj(std::string model, std::string mtl_dir)
{
	std::string warn;
	std::string err;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::material_t> materials;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model.c_str(), mtl_dir.c_str(), true);
	if(!warn.empty()) {
		DEBUG(warn.c_str());
	}

	if (!err.empty()) {
		FATAL(err.c_str());
	}

	for (tinyobj::material_t& material : materials)
	{
		cv::Mat tex=cv::imread(mtl_dir +"/"+material.diffuse_texname, cv::IMREAD_UNCHANGED);
		if (tex.empty()) {
			FATAL(("can not load material:"+ material.diffuse_texname).c_str());
		}
		if(tex.channels()==3){
			cv::cvtColor(tex,tex,cv::COLOR_BGR2BGRA);
		}
		diffuseTextures.emplace_back(tex);
	}
	size_t faceNum=0;
	for (tinyobj::shape_t& shape : shapes) {
		faceNum+=shape.mesh.num_face_vertices.size();
	}
	materalCount = materials.size();
	num_faces=faceNum;
	int pointCount = attrib.vertices.size() / 3;
	vertices=make_aligned_array<cv::Vec4f>(32, sizeof(cv::Vec4f)*pointCount);
	copyV3fTo4f(attrib.vertices.data(), vertices, pointCount);
	uvs = make_aligned_array<cv::Vec2f>(32,sizeof(cv::Vec2f)*pointCount);
	copyV2fTo2f(attrib.texcoords.data(),uvs,pointCount);
	normals=make_aligned_array<cv::Vec4f>(32, sizeof(cv::Vec4f)*pointCount);
	copyV3fTo4f(attrib.normals.data(),normals,pointCount);
	for (size_t i = 0; i < pointCount; i++)
	{
		if(attrib.normals[3*i]!=normals[i][0] ||attrib.normals[3*i+1]!=normals[i][1]){
			cout<<"diff p:"<<i<<endl;
			break;
		}
	}
	if(materalCount<=0){ //这个时候加载顶点颜色
		verticeColors=make_aligned_array<cv::Vec4f>(32, sizeof(cv::Vec4f)*pointCount);
		copyV3fTo4f(attrib.colors.data(),verticeColors,pointCount);
	}
	vertCount=pointCount;
	return ret;
}

int ObjModel::getNumFaces(){
	return num_faces;
}

cv::Vec4f& ObjModel::getVertex(int faceId,int subId){
	size_t index=faceId*3+subId;
	return vertices[index];
}

cv::Vec2f& ObjModel::getUV(int faceId,int subId){
	size_t index=faceId*3+subId;
	return uvs[index];
}

void ObjModel::copyV3fTo4f(float * src, std::unique_ptr<cv::Vec4f[], free_delete> & dst, int vCount)
{
	static __m256i srcIdx = _mm256_setr_epi32(0, 1, 2, 3, 3, 4, 5, 6);
	static __m256 cOne = _mm256_set1_ps(1.0f);
	float* pDst = reinterpret_cast<float*>(dst.get());
	int i = 0;
	for (i = 0; i < vCount; i += 2) {
		__m256 srcData = _mm256_loadu_ps(src);
		__m256 xyzwD = _mm256_blend_ps(_mm256_permutevar8x32_ps(srcData, srcIdx),cOne,0x88);
		_mm256_store_ps(pDst, xyzwD);
		src += 6;
		pDst += 8;
	}
	if (i != vCount) {
		memcpy(pDst, src, 3 * sizeof(float));
		pDst += 3;
		*pDst = 1.0f;
	}
}

void ObjModel::copyV2fTo2f(float* src,std::unique_ptr<cv::Vec2f[], free_delete>& dst,int vCount){
	int i=0;
	float* pDst = reinterpret_cast<float*>(dst.get());
	for (i = 0; i < vCount; i += 4) {
		__m256 srcData=_mm256_loadu_ps(src);
		_mm256_store_ps(pDst,srcData);
		src+=8;
		pDst+=8;
	}
	while(i<vCount){
		*pDst=*src;
		*(pDst+1)=*(src+1);
		i++;
	}
}

//花费了0.018364秒
std::unique_ptr<cv::Vec4f[],free_delete> ObjModel::tranVertices(cv::Mat& tMat)
{
	auto result=make_aligned_array<cv::Vec4f>(32,sizeof(cv::Vec4f)*vertCount);
	int i=0;
	__m128 row0=_mm_loadu_ps(tMat.ptr<float>(0));
	__m256 row0_256=_mm256_insertf128_ps(_mm256_castps128_ps256(row0),row0,1);
	__m128 row1=_mm_loadu_ps(tMat.ptr<float>(1));
	__m256 row1_256=_mm256_insertf128_ps(_mm256_castps128_ps256(row1),row1,1);
	__m128 row2=_mm_loadu_ps(tMat.ptr<float>(2));
	__m256 row2_256=_mm256_insertf128_ps(_mm256_castps128_ps256(row2),row2,1);
	__m128 row3=_mm_loadu_ps(tMat.ptr<float>(3));
	__m256 row3_256=_mm256_insertf128_ps(_mm256_castps128_ps256(row3),row3,1);
	float* pSrc=reinterpret_cast<float*>(vertices.get());
	float* pDst=reinterpret_cast<float*>(result.get());
	for	(i=0;i<vertCount;i+=2){
		__m256 srcData=_mm256_load_ps(pSrc);
		__m256 x=_mm256_dp_ps(row0_256,srcData,0xF1);
		__m256 y=_mm256_dp_ps(row1_256,srcData,0xF2);
		__m256 z=_mm256_dp_ps(row2_256,srcData,0xF4);
		__m256 w=_mm256_dp_ps(row3_256,srcData,0xF8);
		__m256 xyzw=_mm256_add_ps(_mm256_add_ps(_mm256_add_ps(x,y),z),w);
		_mm256_store_ps(pDst,xyzw);
		pSrc+=8;
		pDst+=8;
	}
	if(i!=vertCount){
		__m128 srcData=_mm_load_ps(pSrc);
		__m128 x=_mm_dp_ps(row0,srcData,0xF1);
		__m128 y=_mm_dp_ps(row1,srcData,0xF2);
		__m128 z=_mm_dp_ps(row2,srcData,0xF4);
		__m128 w=_mm_dp_ps(row3,srcData,0xF8);
		__m128 xyzw=_mm_add_ps(_mm_add_ps(_mm_add_ps(x,y),z),w);
		_mm_store_ps(pDst,xyzw);
		i++;
	}
	return std::move(result);
}

std::vector<tinyobj::shape_t>& ObjModel::getShapes(){
	return shapes;
}

cv::Vec4b ObjModel::getMixColor(std::vector<int>& ids, cv::Vec3f& bc_screen)
{
	cv::Vec4b bgra(0,0,0,255);
	/*
	for (int i = 0; i < 3; i++)
	{
		int c = (int)(255 * (attrib.colors[3 * ids[0] + i] * bc_screen[0] +
			attrib.colors[3 * ids[1] + i] * bc_screen[1] +
			attrib.colors[3 * ids[2] + i] * bc_screen[2]));
		bgra[2 - i] = c > 255 ? 255 : c;
	}
	*/
	return bgra;
}

cv::Vec4b ObjModel::getTextureColor(int materialId,std::vector<int>& tids, cv::Vec3f& bc_screen)
{
	cv::Vec4b bgra(0, 0, 0, 255);
	cv::Mat& tex = diffuseTextures[materialId];
	int width = tex.cols;
	int height = tex.rows;
	float u = 0.f, v = 0.f;
	u = uvs[tids[0]][0] * bc_screen[0] +
		uvs[tids[1]][0] * bc_screen[1] +
		uvs[tids[2]][0] * bc_screen[2];
	v= uvs[tids[0]][1] * bc_screen[0] +
		uvs[tids[1]][1] * bc_screen[1] +
		uvs[tids[2]][1] * bc_screen[2];
	int x= std::min(int(u * width + 0.5), width - 1);
	int y = std::min(int((1.0-v) * height + 0.5), height - 1);
	bgra = tex.at<cv::Vec4b>(y, x);
	return bgra;
}
