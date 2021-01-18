#include "render.h"
#include "log.h"
#include <algorithm>
#include <cmath>

void draw_point(cv::Mat& image, int row, int col, unsigned char color) {
    if (row < 0 || col < 0 || row >= image.rows || col >= image.cols) {
        return;
    } else {
        image.at<cv::Vec4b>(row,col)=cv::Vec4b(0,0,color,255);
    }
}

void draw_line(cv::Mat& image, int x0, int y0, int x1, int y1) {
    if (abs(x1 - x0) > abs(y1 - y0)) {
        int x;
        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        for (x = x0; x <= x1; x++) {
            double t = (x - x0) / (double)(x1 - x0);
            int y =  (int)(y0 + (y1 - y0) * t);
            draw_point(image, y, x, 255);
        }
    } else {
        int y;
        if (y0 > y1) {
            std::swap(y0, y1);
            std::swap(x0, x1);
        }
        for (y = y0; y <= y1; y++) {
            double t = (y - y0) / (double)(y1 - y0);
            int x = (int)(x0 + (x1 - x0) * t);
            draw_point(image, y, x, 255);
        }
    }
}

void drawModel(ObjModel& model,cv::Mat& image){
    int numFaces=model.getNumFaces();
    int width=image.cols;
    int height=image.rows;
    int i, j;

    cv::Vec3f light = {0, 0, -1};
    cv::Mat zbuffer(height,width,CV_32FC1,cv::Scalar(FLT_MIN));
    for (i = 0; i < numFaces; i++) {
        cv::Vec3f points[3];
        cv::Vec3f coords[3];
        cv::Vec3f normal;
        cv::Vec2f uvs[3];
        float intensity;
        for (j = 0; j < 3; j++) {
            cv::Vec4f& vertex = model.getVertex(i, j);
            points[j][0] = (int)((vertex[0] + 1) / 2 * (width - 1));
            points[j][1] = (int)((vertex[1] + 1) / 2 * (height - 1));
            points[j][1] = (height - 1) - points[j][1];
            points[j][2]=vertex[2];

            coords[j] = cv::Vec3f(vertex[0], vertex[1], vertex[2]);
            uvs[j]=model.getUV(i,j);
			uvs[j][1] = 1.0 - uvs[j][1];
        }
        normal=(coords[2]-coords[0]).cross((coords[1]-coords[0]));
        normal/=cv::norm(normal);
        intensity = normal.dot(light);
        if (intensity > 0) {
            //fill_triangle(image, points[0], points[1], points[2],uvs[0],uvs[1],uvs[2],zbuffer,model.getTexture(),intensity);
        }
    }
}

void draw_point(cv::Mat& image, cv::Vec2i& point, cv::Vec4b& color) {
    if (point[0] < 0 || point[1] < 0 || point[1] >= image.rows || point[0] >= image.cols) {
        return;
    } else {
        image.at<cv::Vec4b>(point[1],point[0])=color;
    }
}


/*
 * using barycentric coordinates, see http://blackpawn.com/texts/pointinpoly/
 * solve P = A + sAB + tAC
 *   --> AP = sAB + tAC
 *   --> s = (AC.y * AP.x - AC.x * AP.y) / (AB.x * AC.y - AB.y * AC.x)
 *   --> t = (AB.x * AP.y - AB.y * AP.x) / (AB.x * AC.y - AB.y * AC.x)
 * check if the point is in triangle: (s >= 0) && (t >= 0) && (s + t <= 1)
 */
cv::Vec3f barycentric(cv::Vec4f& A,cv::Vec4f& B,cv::Vec4f& C,cv::Vec4f& P){
    /*
    cv::Vec4f AB = B-A;
    cv::Vec4f AC = C-A;
    cv::Vec4f AP = P-A;
	float s, t;

    int denom = AB[0]* AC[1] - AB[1] * AC[0];
    if(denom == 0) {
        return cv::Vec3f(-1,1,1);
    }
    s = static_cast<float>((AC[1]* AP[0] - AC[0] * AP[1]) / (double)denom);
    t = static_cast<float>((AB[0] * AP[1] - AB[1] * AP[0]) / (double)denom);
    return cv::Vec3f{1.0f-(s+t),s,t};
    */
   cv::Vec3f s[2];
	for (int i = 2; i--; ) {
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	cv::Vec3f u = s[0].cross(s[1]);
	if (std::abs(u[2]) > 1e-6) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return cv::Vec3f(1.f - (u[0] + u[1]) / u[2], u[1] / u[2], u[0] / u[2]);
	return cv::Vec3f(-1, 1, 1);
}

RenderContext::RenderContext(int width, int height)
{
    this->width=width;
    this->height=height;
	framebuffer.create(height, width, CV_8UC4);
	depthbuffer.create(height,width,CV_32FC1);
	clearDepthBuffer();
}

RenderContext::~RenderContext()
{
}

void RenderContext::clearDepthBuffer()
{
	depthbuffer.setTo(cv::Scalar(-std::numeric_limits<float>::max()));
}

void RenderContext::drawModel(ObjModel & model, cv::Mat & transform)
{
    auto vertices=model.tranVertices(transform);
    std::vector<cv::Vec4f> pts(3);
	std::vector<int> vids(3);
	std::vector<int> tids(3);
	cv::Vec2f clamp(width - 1, height - 1);
    for(auto& shape:model.getShapes()){
        for (int faceId = 0; faceId < shape.mesh.num_face_vertices.size(); faceId++) {
            int faceIdx3 = 3 * faceId;
            int meterail_id = shape.mesh.material_ids[faceId];
            for (int i = 0; i < 3; i++) {
				int idx = shape.mesh.indices[faceIdx3 + i].vertex_index;
				vids[i] = idx;
				tids[i] = shape.mesh.indices[faceIdx3 + i].texcoord_index;
				pts[i] = vertices[idx];
			}
            cv::Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
			cv::Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
            for (int k = 0; k < 3; k++) {
				for (int m = 0; m < 2; m++) {
					bboxmin[m] = std::max(0.f, std::min(bboxmin[m], round(pts[k][m])));
					bboxmax[m] = std::min(clamp[m], std::max(bboxmax[m], round(pts[k][m])));
				}
			}
            cv::Vec4f P;
            for (P[0] = bboxmin[0]; P[0] <= bboxmax[0]; P[0]++) {
				for (P[1] = bboxmin[1]; P[1] <= bboxmax[1]; P[1]++) {
                    cv::Vec3f bc_screen;
					P[2] = 0;
                    bc_screen = barycentric(pts[0], pts[1], pts[2], P);
                    if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0)
						continue;
                    for (int i = 0; i < 3; i++) P[2] += pts[i][2] * bc_screen[i];
                    if (depthbuffer.at<float>(P[1],P[0]) < P[2]) {
                        cv::Vec4b& bgra = framebuffer.at<cv::Vec4b>(P[1], P[0]);
                        if (meterail_id >= 0) {
                            cv::Vec4b color= model.getTextureColor(meterail_id, tids, bc_screen);
							if (color[3] == 255) {
								bgra = color;
								depthbuffer.at<float>(P[1], P[0]) = P[2];
							}
                            else if(color[3]>0 && color[3]<255)
							{
                                long id = P[1] * width + P[0];
								auto search = blendmap.find(id);
                                if (search != blendmap.end()) {
									std::vector<DepthColor>& colors = search->second;
									auto it = colors.begin();
									DepthColor dc;
									dc.color = color;
									dc.depth = P[2];
									bool inserted = false;
									bool skip = false;
                                    while (it!=colors.end())
									{
										if (abs(it->depth - P[2]) < 1e-4) {
											skip = true;
											break;
										}
										if (it->depth < P[2]) {
											colors.insert(it, dc);
											inserted = true;
											break;
										}
										it++;
									}
									if (!skip&&!inserted) {
										colors.emplace_back(dc);
									}
                                }
                                else{
                                    std::vector<DepthColor> colors(1);
									colors[0].color = color;
									colors[0].depth = P[2];
									blendmap.emplace(std::make_pair(id,colors));
                                }
                            }
                            else {
							    bgra = model.getMixColor(vids, bc_screen);
							    depthbuffer.at<float>(P[1], P[0]) = P[2];
						    }
                        }
                    }
                }
            }
        }
    }
    blendAlpha();
}

void RenderContext::blendAlpha(){
    auto iter = blendmap.begin();
	int width = framebuffer.cols;
	while (iter !=blendmap.end())
	{
		auto& p = iter->first;
		int col = p % width;
		int row = p / width;
		cv::Vec4b& bgra = framebuffer.at<cv::Vec4b>(row, col);
		float& depth = depthbuffer.at<float>(row, col);
		auto& colors = iter->second;
		auto c_iter = colors.begin();
		while (c_iter!=colors.end())
		{
			if (c_iter->depth >= depth) {
				float alpha = c_iter->color[3] / 255.0;
				bgra = bgra * (1 - alpha) + alpha * c_iter->color;
			}
			c_iter++;
		}
		bgra[3] = 255;
		iter++;
	}
}

void RenderContext::writeTo(std::string filename){
    cv::imwrite(filename,framebuffer);
}
