#include "render.h"
#include "log.h"
#include <algorithm>
#include <cmath>

using namespace std;

static inline cv::Vec3f barycentric(cv::Vec2f &AB, cv::Vec2f &pAC, cv::Vec4f &A, cv::Vec3f &P, float &w)
{
    cv::Vec2f PA(A[0] - P[0], A[1] - P[1]);
    float u = -pAC.dot(PA);
    float v = PA[0] * AB[1] - PA[1] * AB[0];
    return cv::Vec3f(1 - (u + v) * w, u * w, v * w);
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

void RenderContext::drawOccluder(ObjModel &model, cv::Mat &transform)
{
    auto vertices = model.tranVertices(transform);
    auto normals = model.tranVertices(transform);
    alignas(32) static cv::Vec4f vs[3];
    alignas(32) static cv::Vec4f vns[3];
    for (auto &shape : model.getShapes())
    {
        for (int faceId = 0; faceId < shape.mesh.num_face_vertices.size(); faceId++)
        {
            int faceIdx3 = 3 * faceId;
            int meterail_id = shape.mesh.material_ids[faceId];
            for (int i = 0; i < 3; i++)
            {
                int idx = shape.mesh.indices[faceIdx3 + i].vertex_index;
                int tid = shape.mesh.indices[faceIdx3 + i].texcoord_index;
                int nId = shape.mesh.indices[faceIdx3 + i].normal_index;
                vns[i] = normals[nId];
                vs[i] = vertices[idx];
            }
            renderTriangle(vs, vns);
        }
    }
}

void RenderContext::drawModel(ObjModel &model, cv::Mat &transform)
{
    auto vertices = model.tranVertices(transform);
    auto normals = model.tranVertices(transform);
    alignas(32) static cv::Vec4f vs[3];
    alignas(32) static cv::Vec4f vns[3];
    alignas(32) static cv::Vec2f uvs[3];
    alignas(32) static cv::Vec4f colors[3];
    for (auto &shape : model.getShapes())
    {
        for (int faceId = 0; faceId < shape.mesh.num_face_vertices.size(); faceId++)
        {
            int faceIdx3 = 3 * faceId;
            int meterail_id = shape.mesh.material_ids[faceId];
            for (int i = 0; i < 3; i++)
            {
                int idx = shape.mesh.indices[faceIdx3 + i].vertex_index;
                int tid = shape.mesh.indices[faceIdx3 + i].texcoord_index;
                int nId = shape.mesh.indices[faceIdx3 + i].normal_index;
                vns[i] = normals[nId];
                vs[i] = vertices[idx];
                if (meterail_id >= 0)
                {
                    uvs[i] = model.getUV(tid);
                }
                else
                {
                    colors[i] = model.getVertColor(idx);
                }
            }

            if (meterail_id >= 0)
            {
                renderTriangle(vs, vns, uvs, model.getMaterial(meterail_id));
            }
            else
            {
                renderTriangle(vs, vns, colors);
            }
        }
    }
    blendAlpha();
}

static inline void computeBox(cv::Vec4f (&vs)[3], cv::Vec2f &bboxmin, cv::Vec2f &bboxmax, cv::Vec2f &clamp)
{
    bboxmin[0] = std::max(std::min(std::min(round(vs[0][0]), round(vs[1][0])), round(vs[2][0])), 0.0f);
    bboxmin[1] = std::max(std::min(std::min(round(vs[0][1]), round(vs[1][1])), round(vs[2][1])), 0.0f);
    bboxmax[0] = std::min(std::max(std::max(round(vs[0][0]), round(vs[1][0])), round(vs[2][0])), clamp[0]);
    bboxmax[1] = std::min(std::max(std::max(round(vs[0][1]), round(vs[1][1])), round(vs[2][1])), clamp[1]);
}

void RenderContext::renderTriangle(cv::Vec4f (&vs)[3], cv::Vec4f (&vns)[3], cv::Vec4f *colors)
{
    static cv::Vec2f clamp(width - 1, height - 1);
    cv::Vec2f bboxmin;
    cv::Vec2f bboxmax;
    computeBox(vs, bboxmin, bboxmax, clamp);
    cv::Vec3f P;
    cv::Vec2f AB(vs[1][0] - vs[0][0], vs[1][1] - vs[0][1]);
    cv::Vec2f pAC(vs[2][1] - vs[0][1], vs[0][0] - vs[2][0]);
    float w = AB.dot(pAC);
    if (abs(w) < 1e-6) //无法计算重心
    {
        return;
    }
    w = 1 / w;
    //预先计算的相关变量
    for (float x = bboxmin[0]; x <= bboxmax[0]; x++)
    {
        for (float y = bboxmin[1]; y <= bboxmax[1]; y++)
        {
            P[0] = x;
            P[1] = y;
            auto bc = barycentric(AB, pAC, vs[0], P, w);
            if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0)
                continue;
            float normZ = vns[0][2] * bc[0] + vns[1][2] * bc[1] + vns[2][2] * bc[2];
            if (normZ > 0) //back face culling
                continue;
            P[2] = vs[0][2] * bc[0] + vs[1][2] * bc[1] + vs[2][2] * bc[2];
            if (depthbuffer.at<float>(P[1], P[0]) < P[2])
            {
                if (colors != nullptr)
                {
                    cv::Vec4b &bgra = framebuffer.at<cv::Vec4b>(P[1], P[0]);
                    cv::Vec4f c = (colors[0] * bc[0] + colors[1] * bc[1] + colors[2] * bc[2]) * 255;
                    bgra[2] = cv::saturate_cast<uchar>(c[0]);
                    bgra[1] = cv::saturate_cast<uchar>(c[1]);
                    bgra[0] = cv::saturate_cast<uchar>(c[2]);
                    bgra[3] = 255;
                }
                depthbuffer.at<float>(P[1], P[0]) = P[2];
            }
        }
    }
}

void RenderContext::renderTriangle(cv::Vec4f (&vs)[3], cv::Vec4f (&vns)[3], cv::Vec2f (&uvs)[3], cv::Mat &texture)
{
    static cv::Vec2f clamp(width - 1, height - 1);
    cv::Vec2f bboxmin;
    cv::Vec2f bboxmax;
    computeBox(vs, bboxmin, bboxmax, clamp);
    cv::Vec3f P;
    cv::Vec2f AB(vs[1][0] - vs[0][0], vs[1][1] - vs[0][1]);
    cv::Vec2f pAC(vs[2][1] - vs[0][1], vs[0][0] - vs[2][0]);
    float w = AB.dot(pAC);
    if (abs(w) < 1e-6) //无法计算重心
    {
        return;
    }
    w = 1 / w;
    //预先计算的相关变量
    for (float x = bboxmin[0]; x <= bboxmax[0]; x++)
    {
        for (float y = bboxmin[1]; y <= bboxmax[1]; y++)
        {
            P[0] = x;
            P[1] = y;
            auto bc = barycentric(AB, pAC, vs[0], P, w);
            if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0)
                continue;
            float normZ = vns[0][2] * bc[0] + vns[1][2] * bc[1] + vns[2][2] * bc[2];
            if (normZ > 0) //back face culling
                continue;
            P[2] = vs[0][2] * bc[0] + vs[1][2] * bc[1] + vs[2][2] * bc[2];
            drawPoint(P, bc, uvs, texture);
        }
    }
}

void RenderContext::drawPoint(cv::Vec3f &P, cv::Vec3f &bc, cv::Vec2f (&tex_uv)[3], cv::Mat &texture)
{
    if (depthbuffer.at<float>(P[1], P[0]) < P[2])
    {
        cv::Vec4b &bgra = framebuffer.at<cv::Vec4b>(P[1], P[0]);
        float u = tex_uv[0][0] * bc[0] + tex_uv[1][0] * bc[1] + tex_uv[2][0] * bc[2];
        float v = tex_uv[0][1] * bc[0] + tex_uv[1][1] * bc[1] + tex_uv[2][1] * bc[2];
        int tx = std::min(int(u * texture.cols + 0.5), texture.cols - 1);
        int ty = std::min(int((1.0 - v) * texture.rows + 0.5), texture.rows - 1);
        cv::Vec4b &color = texture.at<cv::Vec4b>(ty, tx);
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
