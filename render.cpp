#include "render.h"
#include "log.h"

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
    for (i = 0; i < numFaces; i++) {
        for (j = 0; j < 3; j++) {
            vec3f_t& v0=model.getVertex(i,j);
            vec3f_t& v1=model.getVertex(i,(j+1)%3);
            int x0 = (int)((v0.x + 1) / 2 * width);
            int y0 = (int)((v0.y + 1) / 2 * height);
            int x1 = (int)((v1.x + 1) / 2 * width);
            int y1 = (int)((v1.y + 1) / 2 * height);

            draw_line(image, x0, y0, x1, y1);
        }
    }
	cv::flip(image, image, 0);
}

void sort_point_y(vec2i_t& point0,vec2i_t& point1,vec2i_t& point2){
    if(point0.y>point1.y){
        std::swap(point0,point1);
    }
    if(point0.y>point2.y){
        std::swap(point0,point2);
    }
    if(point1.y>point2.y){
        std::swap(point1,point2);
    }
}

void sort_point_x(vec2i_t& point0,vec2i_t& point1,vec2i_t& point2){
    if(point0.x>point1.x){
        std::swap(point0,point1);
    }
    if(point0.x>point2.x){
        std::swap(point0,point2);
    }
    if(point1.x>point2.x){
        std::swap(point1,point2);
    }
}

void draw_point(cv::Mat& image, vec2i_t& point, cv::Vec4b& color) {
    if (point.x < 0 || point.y < 0 || point.y >= image.rows || point.x >= image.cols) {
        return;
    } else {
        image.at<cv::Vec4b>(point.y,point.x)=color;
    }
}

static void draw_scanline(cv::Mat& image,vec2i_t& point0,vec2i_t& point1,
                          cv::Vec4b& color) {
    vec2i_t point;
    FORCE(point0.y == point1.y, "draw_scanline: diff y");
    if (point0.x > point1.x) {
        std::swap(point0, point1);
    }
    for (point = point0; point.x <= point1.x; point.x += 1) {
        draw_point(image, point, color);
    }
}

static int linear_interp(int& v0, int& v1, double& d) {
    return (int)(v0 + (v1 - v0) * d + 0.5);
}

static vec2i_t lerp_point(vec2i_t& point0, vec2i_t& point1, double& d) {
    vec2i_t point;
    point.x = linear_interp(point0.x, point1.x, d);
    point.y = linear_interp(point0.y, point1.y, d);
    return point;
}

void fill_triangle(cv::Mat& image,vec2i_t& point0,vec2i_t& point1,vec2i_t& point2,cv::Vec4b& color){
    sort_point_y(point0,point1,point2);
    if(point0.y==point2.y){ //共线
        sort_point_x(point0,point1,point2);
        draw_scanline(image,point0,point2,color);
    }else
    {
        int total_height = point2.y - point0.y;
        int upper_height = point1.y - point0.y;
        int lower_height = point2.y - point1.y;

        if (upper_height == 0) {
            draw_scanline(image, point0, point1, color);
        } else {
            int y;
            for (y = point0.y; y <= point1.y; y++) {
                double d1 = (y - point0.y) / (double)upper_height;
                double d2 = (y - point0.y) / (double)total_height;
                vec2i_t p1 = lerp_point(point0, point1, d1);
                vec2i_t p2 = lerp_point(point0, point2, d2);
                p1.y = p2.y = y;
                draw_scanline(image, p1, p2, color);
            }
        }

        if (lower_height == 0) {
            draw_scanline(image, point1, point2, color);
        } else {
            int y;
            for (y = point1.y; y <= point2.y; y++) {
                double d0 = (y - point0.y) / (double)total_height;
                double d1 = (y - point1.y) / (double)lower_height;
                vec2i_t p0 = lerp_point(point0, point2, d0);
                vec2i_t p1 = lerp_point(point1, point2, d1);
                p0.y = p1.y = y;
                draw_scanline(image, p0, p1, color);
            }
        }
    }  
}