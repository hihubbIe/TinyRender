#pragma once

#include "geometry.h"
#include "tgaimage.h"


const int depth = 255;
const int width = 3000;
const int height = 3000;
const bool aa = false;

struct IShader {
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor& color) = 0;
};

Matrix proj(float coeff = 0.f);
Matrix viewport(int x, int y, int w, int h);
Matrix lookat(Vec3f eye, Vec3f center, Vec3f up);
void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color);
void triangle(Vec4f* points, IShader* shader, float* zbuffer, TGAImage& image);