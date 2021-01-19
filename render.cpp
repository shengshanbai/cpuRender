#include "render.h"
#include "log.h"
#include <algorithm>
#include <cmath>

using namespace std;

bool barycentric(const __m128 &v0, const __m128 &v1, const __m128 &v2, cv::Vec4f &P, __m128 &v_bc)
{
    static __m128 arg = _mm_setr_ps(1.0f, -1.0f, 0.0f, 0.0f);
    static __m128 narg = _mm_setr_ps(-1.0f, 1.0f, 0.0f, 0.0f);
    static __m128 multi_uv = _mm_setr_ps(0.0f, 1.0f, 1.0f, 0.0f);
    __m128 p = _mm_loadu_ps(reinterpret_cast<float *>(&P));
    __m128 AB = _mm_sub_ps(v1, v0);
    __m128 AC = _mm_sub_ps(v2, v0);
    __m128 PA = _mm_sub_ps(v0, p);
    __m128 per_pa = _mm_permute_ps(PA, 0x01);
    __m128 per_ac = _mm_permute_ps(AC, 0x01);
    __m128 x = _mm_dp_ps(_mm_mul_ps(AC, per_pa), arg, 0x31);
    __m128 y = _mm_dp_ps(_mm_mul_ps(AB, per_pa), narg, 0x32);
    __m128 z = _mm_dp_ps(_mm_mul_ps(AB, per_ac), arg, 0x34);
    __m128 xyz = _mm_add_ps(_mm_add_ps(x, y), z);
    float weight;
    _MM_EXTRACT_FLOAT(weight, xyz, 2);
    if (abs(weight) > 1e-06)
    {
        __m128 v_1uv = _mm_permute_ps(_mm_div_ps(xyz, _mm_set1_ps(weight)), 0xD2);
        __m128 sum = _mm_dp_ps(v_1uv, multi_uv, 0x61);
        v_bc = _mm_sub_ps(v_1uv, sum);
        return true;
    }
    else
    {
        v_bc = _mm_setr_ps(-1.0f, 1.0f, 1.0f, 0.0f);
        return false;
    }
}

cv::Vec3f barycentric(cv::Vec4f &A, cv::Vec4f &B, cv::Vec4f &C, cv::Vec4f &P)
{
    cv::Vec3f s[2];
    for (int i = 2; i--;)
    {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    cv::Vec3f u = s[0].cross(s[1]);
    if (std::abs(u[2]) > 1e-6) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return cv::Vec3f(1.f - (u[0] + u[1]) / u[2], u[1] / u[2], u[0] / u[2]);
    return cv::Vec3f(-1, 1, 1);
}

bool inTriangle(const __m128 &v_bc)
{
    float x, y, z;
    _MM_EXTRACT_FLOAT(x, v_bc, 0);
    _MM_EXTRACT_FLOAT(y, v_bc, 1);
    _MM_EXTRACT_FLOAT(z, v_bc, 2);
    return (x >= 0 && y >= 0 && z >= 0);
}

RenderContext::RenderContext(int width, int height)
{
    _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
    this->width = width;
    this->height = height;
    framebuffer.create(height, width, CV_8UC4);
    depthbuffer.create(height, width, CV_32FC1);
    clearDepthBuffer();
}

RenderContext::~RenderContext()
{
}

void RenderContext::clearDepthBuffer()
{
    depthbuffer.setTo(cv::Scalar(-std::numeric_limits<float>::max()));
}

void RenderContext::drawModel(ObjModel &model, cv::Mat &transform)
{
    auto vertices = model.tranVertices(transform);
    auto normals = model.tranVertices(transform);
    alignas(32) static cv::Vec4f vs[3];
    alignas(32) static cv::Vec4f vns[3];
    alignas(32) static cv::Vec2f uvs[3];
    static int tids[3];
    static int vids[3];
    cv::Vec2f clamp(width - 1, height - 1);
    for (auto &shape : model.getShapes())
    {
        for (int faceId = 0; faceId < shape.mesh.num_face_vertices.size(); faceId++)
        {
            int faceIdx3 = 3 * faceId;
            int meterail_id = shape.mesh.material_ids[faceId];
            for (int i = 0; i < 3; i++)
            {
                int idx = shape.mesh.indices[faceIdx3 + i].vertex_index;
                vids[i] = idx;
                int tid = shape.mesh.indices[faceIdx3 + i].texcoord_index;
                tids[i]=tid;
                vs[i] = vertices[idx];
                int nId = shape.mesh.indices[faceIdx3 + i].normal_index;
                vns[i] = normals[nId];
                uvs[i] = model.getUV(tid);
            }
            renderTriangle(vs, vns, uvs, model.getMaterial(meterail_id));
        }
    }
    blendAlpha();
}

bool isBackFace(cv::Vec4f (&vns)[3], const __m128 &v_bc)
{
    __m128 z = _mm_setr_ps(vns[0][2], vns[1][2], vns[2][2], 0.0f);
    z = _mm_dp_ps(z, v_bc, 0xF1);
    float wz;
    _MM_EXTRACT_FLOAT(wz, z, 0);
    return wz < 0;
}

float getDepth(const __m128 &v0, const __m128 &v1, const __m128 &v2, const __m128 &v_bc)
{
    __m128 z01 = _mm_shuffle_ps(v0, v1, 0xAA);
    __m128 z = _mm_shuffle_ps(z01, v2, 0xA8);
    __m128 dep = _mm_dp_ps(v_bc, z, 0x71);
    float depth;
    _MM_EXTRACT_FLOAT(depth, dep, 0);
    return depth;
}

cv::Vec4b getTextColor(cv::Mat &texture, const __m256 &uvs, const __m128 &v_bc, const __m128 &t_width, const __m128 &t_height)
{
    __m256i uv_idx = _mm256_setr_epi32(0x0, 0x2, 0x4, 0x4, 0x1, 0x3, 0x5, 0x5);
    __m256 uuuvvv = _mm256_permutevar8x32_ps(uvs, uv_idx);
    __m128 us = _mm256_extractf128_ps(uuuvvv, 0x0);
    __m128 vs = _mm256_extractf128_ps(uuuvvv, 0x1);
    us = _mm_round_ps(_mm_min_ps(_mm_mul_ps(_mm_dp_ps(us, v_bc, 0x71), t_width), t_width), (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
    vs = _mm_round_ps(_mm_min_ps(_mm_mul_ps(_mm_dp_ps(vs, v_bc, 0x71), t_height), t_height), (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
    float u, v;
    _MM_EXTRACT_FLOAT(u, us, 0);
    _MM_EXTRACT_FLOAT(v, vs, 0);
    return texture.at<cv::Vec4b>(static_cast<int>(v), static_cast<int>(u));
}

void RenderContext::renderTriangle(cv::Vec4f (&vs)[3], cv::Vec4f (&vns)[3], cv::Vec2f (&uvs)[3], cv::Mat &texture)
{
    static cv::Vec2f clamp(width - 1, height - 1);
    cv::Vec2f bboxmin;
    cv::Vec2f bboxmax;
    bboxmin[0]=std::max(std::min(std::min(round(vs[0][0]),round(vs[1][0])),round(vs[2][0])),0.0f);
    bboxmin[1]=std::max(std::min(std::min(round(vs[0][1]),round(vs[1][1])),round(vs[2][1])),0.0f);
    bboxmax[0]=std::min(std::max(std::max(round(vs[0][0]),round(vs[1][0])),round(vs[2][0])),clamp[0]);
    bboxmax[1]=std::min(std::max(std::max(round(vs[0][1]),round(vs[1][1])),round(vs[2][1])),clamp[1]);
    cv::Vec4f P;
    for (P[0] = bboxmin[0]; P[0] <= bboxmax[0]; P[0]++)
    {
        for (P[1] = bboxmin[1]; P[1] <= bboxmax[1]; P[1]++)
        {
            cv::Vec3f bc_screen;
            P[2] = 0;
            bc_screen = barycentric(vs[0], vs[1], vs[2], P);
            if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0)
                continue;
            cv::Vec4f cNorm = vns[0] * bc_screen[0] + vns[1] * bc_screen[1] + vns[2] * bc_screen[2];
            if (cNorm[3] < 0) //back face culling
                continue;
            for (int i = 0; i < 3; i++)
                P[2] += vs[i][2] * bc_screen[i];
            if (depthbuffer.at<float>(P[1], P[0]) < P[2])
            {
                cv::Vec4b &bgra = framebuffer.at<cv::Vec4b>(P[1], P[0]);
                if (true)
                {
                    float u = uvs[0][0] * bc_screen[0] + uvs[1][0] * bc_screen[1] + uvs[2][0] * bc_screen[2];
                    float v = uvs[0][1] * bc_screen[0] + uvs[1][1] * bc_screen[1] + uvs[2][1] * bc_screen[2];
                    int x = std::min(int(u * texture.cols + 0.5), texture.cols - 1);
                    int y = std::min(int((1.0 - v) * texture.rows + 0.5), texture.rows - 1);
                    cv::Vec4b &color = texture.at<cv::Vec4b>(y, x);
                    if (color[3] == 255)
                    {
                        bgra = color;
                        depthbuffer.at<float>(P[1], P[0]) = P[2];
                    }
                    else if (color[3] > 0 && color[3] < 255)
                    {
                        long id = P[1] * width + P[0];
                        auto search = blendmap.find(id);
                        if (search != blendmap.end())
                        {
                            std::vector<DepthColor> &colors = search->second;
                            auto it = colors.begin();
                            DepthColor dc;
                            dc.color = color;
                            dc.depth = P[2];
                            bool inserted = false;
                            bool skip = false;
                            while (it != colors.end())
                            {
                                if (abs(it->depth - P[2]) < 1e-4)
                                {
                                    skip = true;
                                    break;
                                }
                                if (it->depth < P[2])
                                {
                                    colors.insert(it, dc);
                                    inserted = true;
                                    break;
                                }
                                it++;
                            }
                            if (!skip && !inserted)
                            {
                                colors.emplace_back(dc);
                            }
                        }
                        else
                        {
                            std::vector<DepthColor> colors(1);
                            colors[0].color = color;
                            colors[0].depth = P[2];
                            blendmap.emplace(std::make_pair(id, colors));
                        }
                    }
                    else
                    {
                        depthbuffer.at<float>(P[1], P[0]) = P[2];
                    }
                }
            }
        }
    }
}

/*
void RenderContext::renderTriangle(cv::Vec4f (&vs)[3], cv::Vec4f (&vns)[3], cv::Vec2f (&uvs)[3], cv::Mat &texture)
{
    __m256 v0v1 = _mm256_load_ps(reinterpret_cast<float *>(&vs[0]));
    __m128 v0 = _mm256_extractf128_ps(v0v1, 0x0);
    __m128 v1 = _mm256_extractf128_ps(v0v1, 0x1);
    __m128 v2 = _mm_load_ps(reinterpret_cast<float *>(&vs[2]));
    //计算box范围
    static __m128 max_size = _mm_setr_ps(width - 1, height - 1, 0, 0);
    static __m128 min_size = _mm_setr_ps(0, 0, 0, 0);
    __m128 box_min = _mm_round_ps(_mm_max_ps(_mm_min_ps(_mm_min_ps(v0, v1), v2), min_size), (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
    __m128 box_max = _mm_round_ps(_mm_min_ps(_mm_max_ps(_mm_max_ps(v0, v1), v2), max_size), (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
    float minX, minY, maxX, maxY;
    _MM_EXTRACT_FLOAT(minX, box_min, 0);
    _MM_EXTRACT_FLOAT(minY, box_min, 1);
    _MM_EXTRACT_FLOAT(maxX, box_max, 0);
    _MM_EXTRACT_FLOAT(maxY, box_max, 1);
    //遍历点
    static cv::Vec4f P;
    //uv坐标
    __m256 v_uvs = _mm256_load_ps(reinterpret_cast<float *>(uvs));
    __m128 t_width = _mm_set1_ps(texture.cols - 1.0f);
    __m128 t_height = _mm_set1_ps(texture.rows - 1.0f);
    for (P[0] = minX; P[0] <= maxX; P[0]++)
    {
        for (P[1] = minY; P[1] <= maxY; P[1]++)
        {
            __m128 v_bc;
            if (!barycentric(v0, v1, v2, P, v_bc)) //无法计算
                continue;
            if (!inTriangle(v_bc)) //不在三角形中
                continue;
            if (isBackFace(vns, v_bc)) //背面剔除
                continue;
            P[2] = getDepth(v0, v1, v2, v_bc);
            if (depthbuffer.at<float>(P[1], P[0]) < P[2])
            {
                cv::Vec4b &bgra = framebuffer.at<cv::Vec4b>(P[1], P[0]);
                cv::Vec4b color = getTextColor(texture, v_uvs, v_bc, t_width, t_height);
                if (color[3] == 255)
                {
                    bgra = color;
                    depthbuffer.at<float>(P[1], P[0]) = P[2];
                }
                else if (color[3] > 0 && color[3] < 255)
                {
                    long id = P[1] * width + P[0];
                    auto search = blendmap.find(id);
                    if (search != blendmap.end())
                    {
                        std::vector<DepthColor> &colors = search->second;
                        auto it = colors.begin();
                        DepthColor dc;
                        dc.color = color;
                        dc.depth = P[2];
                        bool inserted = false;
                        bool skip = false;
                        while (it != colors.end())
                        {
                            if (abs(it->depth - P[2]) < 1e-4)
                            {
                                skip = true;
                                break;
                            }
                            if (it->depth < P[2])
                            {
                                colors.insert(it, dc);
                                inserted = true;
                                break;
                            }
                            it++;
                        }
                        if (!skip && !inserted)
                        {
                            colors.emplace_back(dc);
                        }
                    }
                    else
                    {
                        std::vector<DepthColor> colors(1);
                        colors[0].color = color;
                        colors[0].depth = P[2];
                        blendmap.emplace(std::make_pair(id, colors));
                    }
                }
            }
        }
    }
}
*/

void RenderContext::blendAlpha()
{
    auto iter = blendmap.begin();
    int width = framebuffer.cols;
    while (iter != blendmap.end())
    {
        auto &p = iter->first;
        int col = p % width;
        int row = p / width;
        cv::Vec4b &bgra = framebuffer.at<cv::Vec4b>(row, col);
        float &depth = depthbuffer.at<float>(row, col);
        auto &colors = iter->second;
        auto c_iter = colors.begin();
        while (c_iter != colors.end())
        {
            if (c_iter->depth >= depth)
            {
                float alpha = c_iter->color[3] / 255.0;
                bgra = bgra * (1 - alpha) + alpha * c_iter->color;
            }
            c_iter++;
        }
        bgra[3] = 255;
        iter++;
    }
}

void RenderContext::writeTo(std::string filename)
{
    cv::imwrite(filename, framebuffer);
}
