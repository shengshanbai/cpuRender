#include "render.h"


void draw_point(cv::Mat& image, int row, int col, unsigned char color) {
    if (row < 0 || col < 0 || row >= image.rows || col >= image.cols) {
        return;
    } else {
        image.at<cv::Vec4b>(row,col)[3]=color;
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
}