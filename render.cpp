#include "render.h"
#include "log.h"
#include <algorithm>

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
cv::Vec3f barycentric(cv::Vec2i& A,cv::Vec2i& B,cv::Vec2i& C,cv::Vec2i& P){
    cv::Vec2i AB = B-A;
    cv::Vec2i AC = C-A;
    cv::Vec2i AP = P-A;
	float s, t;

    int denom = AB[0]* AC[1] - AB[1] * AC[0];
    if(denom == 0) {
        return cv::Vec3f(-1,1,1);
    }
    s = static_cast<float>((AC[1]* AP[0] - AC[0] * AP[1]) / (double)denom);
    t = static_cast<float>((AB[0] * AP[1] - AB[1] * AP[0]) / (double)denom);
    return cv::Vec3f{1.0f-(s+t),s,t};
}

cv::Vec3f barycentric(cv::Vec3f& A,cv::Vec3f& B,cv::Vec3f& C,cv::Vec3f& P){
    cv::Vec3f AB = B-A;
    cv::Vec3f AC = C-A;
    cv::Vec3f AP = P-A;
	float s, t;

    int denom = AB[0]* AC[1] - AB[1] * AC[0];
    if(denom == 0) {
        return cv::Vec3f(-1,1,1);
    }
    s = static_cast<float>((AC[1]* AP[0] - AC[0] * AP[1]) / (double)denom);
    t = static_cast<float>((AB[0] * AP[1] - AB[1] * AP[0]) / (double)denom);
    return cv::Vec3f{1.0f-(s+t),s,t};
}

static int in_triangle(cv::Vec2i& A,cv::Vec2i& B,cv::Vec2i& C,cv::Vec2i& P){
    cv::Vec3f center=barycentric(A,B,C,P);
    return (center[0] >=0 && center[1] >= 0 && center[2]>=0);
}

void drawModel(RenderContext & context, ObjModel & model, cv::Mat & diffuse_map, cv::Mat & normal_map, cv::Mat & specular_map)
{

}

void fill_triangle(cv::Mat& image,cv::Vec3f& point0,cv::Vec3f& point1,cv::Vec3f& point2,
    cv::Vec2f& uv0,cv::Vec2f& uv1,cv::Vec2f& uv2,cv::Mat& zbuffer,cv::Mat& texture,float intensity){
    int width=image.cols;
    int height=image.rows;

    int tex_width=texture.cols;
    int tex_height=texture.rows;

    int minX=static_cast<int>(std::max(std::min(std::min(point0[0],point1[0]),point2[0]),0.0f));
    int minY=static_cast<int>(std::max(std::min(std::min(point0[1],point1[1]),point2[1]),0.0f));
    int maxX=static_cast<int>(std::min(std::max(std::max(point0[0],point1[0]),point2[0]),(float)width-1));
    int maxY=static_cast<int>(std::min(std::max(std::max(point0[1],point1[1]),point2[1]),(float)height-1));
    for (int i = minX; i <=maxX; i++)
    {
        for (int j = minY; j <= maxY; j++)
        {
            cv::Vec3f P(i,j,1);
            cv::Vec3f baryC=barycentric(point0,point1,point2,P);
            if(baryC[0] >=0 && baryC[1] >= 0 && baryC[2]>=0){ //在三角形内部
                float z=baryC[0]*point0[2]+baryC[1]*point1[2]+baryC[2]*point2[2];
                if (zbuffer.at<float>(j,i)<z){
                    float tex_x=baryC[0]*uv0[0]+baryC[1]*uv1[0]+baryC[2]*uv2[0];
                    float tex_y=baryC[0]*uv0[1]+baryC[1]*uv1[1]+baryC[2]*uv2[1];
                    int t_x = std::min(std::max(0,(int)(tex_x * (tex_width - 1))),tex_width-1);
                    int t_y = std::min(std::max(0,(int)(tex_y * (tex_height - 1))),tex_height-1);
                    cv::Vec4b color=texture.at<cv::Vec4b>(t_y,t_x);
                    color[0]=cv::saturate_cast<uchar>(color[0]*intensity);
                    color[1]=cv::saturate_cast<uchar>(color[1]*intensity);
                    color[2]=cv::saturate_cast<uchar>(color[2]*intensity);
                    image.at<cv::Vec4b>(j,i)=color;
                    zbuffer.at<float>(j,i)=z;
                }
            }
        }
    }
}

RenderContext::RenderContext(int width, int height)
{
	framebuffer.create(height, width, CV_8UC4);
	depthbuffer.create(height,width,CV_32FC1);
	clearDepthBuffer();
}

RenderContext::~RenderContext()
{
}

void RenderContext::clearDepthBuffer()
{
	depthbuffer.setTo(cv::Scalar(0.0));
}

void RenderContext::drawModel(ObjModel & model, cv::Mat4f & transform)
{

}
