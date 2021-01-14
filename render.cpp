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

    for (i = 0; i < numFaces; i++) {
        cv::Vec2i points[3];
        cv::Vec3f coords[3];
        cv::Vec3f normal;
        float intensity;
        for (j = 0; j < 3; j++) {
            cv::Vec3f& vertex = model.getVertex(i, j);
            points[j][0] = (int)((vertex[0] + 1) / 2 * (width - 1));
            points[j][1] = (int)((vertex[1] + 1) / 2 * (height - 1));
            points[j][1] = (height - 1) - points[j][1];

            coords[j] = vertex;
        }
        normal=(coords[2]-coords[0]).cross((coords[1]-coords[0]));
        normal/=cv::norm(normal);
        intensity = normal.dot(light);
        if (intensity > 0) {
            cv::Vec4b color;
            color[0] = (unsigned char)(intensity * 255);
            color[1] = (unsigned char)(intensity * 255);
            color[2] = (unsigned char)(intensity * 255);
            color[3] = 255;
            fill_triangle(image, points[0], points[1], points[2], color);
        }
    }
}

void draw_point(cv::Mat& image, cv::Vec2i& point, cv::Vec4b& color) {
    if (point.x < 0 || point.y < 0 || point.y >= image.rows || point.x >= image.cols) {
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
    s = static_cast<float>((AC.y * AP.x - AC.x * AP.y) / (double)denom);
    t = static_cast<float>((AB.x * AP.y - AB.y * AP.x) / (double)denom);
    return cv::Vec3f{1.0f-(s+t),s,t};
}

static int in_triangle(cv::Vec2i& A,cv::Vec2i& B,cv::Vec2i& C,cv::Vec2i& P){
    vec3f_t center=barycentric(A,B,C,P);
    return (center[0] >=0 && center[1] >= 0 && center[2]>=0);
}

void fill_triangle(cv::Mat& image,cv::Vec2i& point0,cv::Vec2i& point1,cv::Vec2i& point2,cv::Vec4b& color){
    int width=image.cols;
    int height=image.rows;
    int minX=1e6;
    int minY=1e6;
    int maxX=0;
    int maxY=0;
    minX=std::max(std::min(std::min(std::min(minX,point0[0]),point1[0]),point2[0]),0);
    minY=std::max(std::min(std::min(std::min(minX,point0[1]),point1[1]),point2[1]),0);
    maxX=std::min(std::max(std::max(std::max(maxX,point0[0]),point1[0]),point2[0]),width-1);
    maxY=std::min(std::max(std::max(std::max(maxX,point0[1]),point1[1]),point2[1]),height-1);
    for (int i = minX; i <=maxX; i++)
    {
        for (int j = minY; j < maxY; j++)
        {
            vec2i_t p={i,j};
            if(in_triangle(point0,point1,point2,p)){
                draw_point(image,p,color);
            }
        }
    }
}