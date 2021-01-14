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
	std::vector<tinyobj::shape_t> shapes;
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
		cv::Mat tex=cv::imread(mtl_dir + material.diffuse_texname, cv::IMREAD_UNCHANGED);
		textures.emplace_back(tex);
	}
	size_t faceNum=0;
	for (tinyobj::shape_t& shape : shapes) {
		faceNum+=shape.mesh.num_face_vertices.size();
	}
	auto pNewVertices=make_aligned_array<vec3f_t>(32,3*faceNum*sizeof(vec3f_t));
	auto pNewUvs=make_aligned_array<vec2f_t>(32,3*faceNum*sizeof(vec2f_t));
	auto pNewNormals=make_aligned_array<vec3f_t>(32,3*faceNum*sizeof(vec3f_t));
	vec3f_t* dVtx=pNewVertices.get();
	vec2f_t* dUv=pNewUvs.get();
	vec3f_t* dNorm=pNewNormals.get();
	for(tinyobj::shape_t& shape : shapes){
		for (int faceId = 0; faceId < shape.mesh.num_face_vertices.size(); faceId++) {
			for(int i=0;i<3;i++){
				int vid=shape.mesh.indices[3*faceId+i].vertex_index;
				int uvid=shape.mesh.indices[3*faceId+i].texcoord_index;
				int normId=shape.mesh.indices[3*faceId+i].normal_index;
				memcpy(dVtx,&attrib.vertices[3*vid],sizeof(vec3f_t));
				memcpy(dUv,&attrib.texcoords[2*uvid],sizeof(vec2f_t));
				memcpy(dNorm,&attrib.normals[3*normId],sizeof(vec3f_t));
				dVtx++;
				dUv++;
				dNorm++;
			}
		}
	}
	vertices.swap(pNewVertices);
	uvs.swap(pNewUvs);
	normals.swap(pNewNormals);
	num_faces=faceNum;
	return ret;
}

int ObjModel::getNumFaces(){
	return num_faces;
}

vec3f_t& ObjModel::getVertex(int faceId,int subId){
	size_t index=faceId*3+subId;
	return vertices[index];
}

//花费了0.018364秒
std::unique_ptr<float[],free_delete> ObjModel::transform(std::unique_ptr<float[],free_delete>& rtMat)
{
	auto result=make_aligned_array<float>(16,4*sizeof(float));
	/*
	float* pMat=rtMat.get();
	__m128 row0=_mm_load_ps(pMat);
	__m128 row1=_mm_load_ps(pMat+4);
	__m128 row2=_mm_load_ps(pMat+8);
	__m128 pickW=_mm_set_ps(0xFFFFFFFF,0x00,0x00,0x00);
	float* pVert=vertices.get();
	float* pDst=result.get();
	for(int i=0;i<verticeCount;i++){
		__m128 xyzw=_mm_load_ps(pVert);
		__m128 newP=_mm_add_ps(_mm_add_ps(_mm_dp_ps(row0,xyzw,0xF1),_mm_dp_ps(row1,xyzw,0xF2)),_mm_add_ps(_mm_dp_ps(row2,xyzw,0xF4),_mm_and_ps(pickW,xyzw)));
		_mm_store_ps(pDst,newP);
		pVert+=4;
		pDst+=4;
	}
	*/
	return std::move(result);
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
	/*
	cv::Vec4b bgra(0, 0, 0, 255);
	cv::Mat& tex = textures[materialId];
	int width = tex.cols;
	int height = tex.rows;
	float u = 0.f, v = 0.f;
	u = attrib.texcoords[2 * tids[0]] * bc_screen[0] +
		attrib.texcoords[2 * tids[1]] * bc_screen[1] +
		attrib.texcoords[2 * tids[2]] * bc_screen[2];
	v= attrib.texcoords[2 * tids[0]+1] * bc_screen[0] +
		attrib.texcoords[2 * tids[1]+1] * bc_screen[1] +
		attrib.texcoords[2 * tids[2]+1] * bc_screen[2];
	int x= std::min(int(u * width + 0.5), width - 1);
	int y = std::min(int((1.0-v) * height + 0.5), height - 1);
	if (tex.channels() == 4) {
		bgra = tex.at<cv::Vec4b>(y, x);
		return bgra;
	}
	else
	{
		cv::Vec3b& color = tex.at<cv::Vec3b>(y, x);
		bgra[0] = color[0];
		bgra[1] = color[1];
		bgra[2] = color[2];
		return bgra;
	}
	*/
	return cv::Vec4b(0, 0, 0, 255);
}
