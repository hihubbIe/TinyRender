// TInyRenderer.cpp : Defines the entry point for the application.
//

#include "cstdlib"
#include "iostream"
#include <cmath>
#include "model.h"
#include "homemade_gl.h"

Vec3f camera(0, 0, 3);
Vec3f eye(1.5f, 0.3f, 2.f);
Vec3f center(0, 0, 0);
Matrix mVP = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
Matrix mProj = Matrix::identity();
Matrix mMV = lookat(eye, center, Vec3f(0, 1, 0));
Model* current_model = 0;
Vec3f light(-2, 1.f, 3);
float* zbuffer;
const float ZBUFFER_MIN = -10;

#ifdef AFRICAN_HEAD_1
const char* files[1] = { "obj/african_head/african_head.obj" }; int file_count = 1; const float focus = 19; const float focus_strength = 1.1f;
#endif
#ifdef AFRICAN_HEAD_3
const char* files[3] = { "obj/african_head/african_head.obj", "obj/african_head/african_head_eye_inner.obj", "obj/african_head/african_head_eye_outer.obj" }; int file_count = 3; const float focus = 19; const float focus_strength = 1.1f;
#endif
#ifdef AFRICAN_HEAD_2
const char* files[2] = { "obj/african_head/african_head.obj", "obj/african_head/african_head_eye_inner.obj" }; int file_count = 2; const float focus = 19; const float focus_strength = 1.1f;
#endif
#ifdef BOGGIE
const char* files[3] = { "obj/boggie/body.obj", "obj/boggie/head.obj", "obj/boggie/eyes.obj" }; int file_count = 3; const float focus = 15; const float focus_strength = 1.4f;
#endif
#ifdef DIABLO3_POSE
const char* files[1] = { "obj/diablo3_pose/diablo3_pose.obj" }; int file_count = 1; const float focus = 16; const float focus_strength = 0.9f;
#endif

void printM(Matrix m) {
    for (int i = 0; i < 4; i++) {
        printf("\n");
        for (int j = 0; j < 4; j++) {
            if (j > 0) printf(", ");
            printf("%g", m[i][j]);
        }
    }
    printf("\n");
}

void printArr(float* arr, int w, int h) {
    for (int i = 0; i < h; i++) {
        printf("\n");
        for (int j = 0; j < w; j++) {
            if (j > 0) printf(", ");
            printf("%6.4lf", arr[i * w + j]);
        }
    }
    printf("\n");
}

struct Shader : public IShader {
    mat<2, 3, float> varying_uv;
    Matrix uniform_M;
    Matrix uniform_MIT;

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, current_model->uv(iface, nthvert));
        Vec4f v = embed<4>(current_model->vert(iface, nthvert)); // read the vertex from .obj file
        return mVP * mProj * mMV * v; // transform it to screen coordinates
    }

    virtual bool fragment(Vec3f bar, TGAColor& color) {
        Vec2f uv = varying_uv * bar;
        Vec3f n = proj<3>(uniform_MIT * embed<4>(current_model->normal(uv))).normalize();
        Vec3f l = proj<3>(uniform_M * embed<4>(light)).normalize();
        Vec3f r = (n * (n * l * 2.f) - l).normalize();
        float specular = pow(std::max(r.z, 0.f), current_model->specular(uv));
        float diffuse = std::max(0.f, n * l);
        color = current_model->diffuse(uv);
        bool isGlow = false;
        float* glowRGB = new float[3];
        glowRGB[0] = 0; glowRGB[1] = 0; glowRGB[2] = 0;
#ifdef DIABLO3_POSE
        TGAColor glow = current_model->glow(uv);
        float glow_intensity = 10;
        glowRGB[2] = std::min((int)(glow.bgra[2] * glow_intensity), 255);
        glowRGB[1] = std::min((int)(glow.bgra[1] * glow_intensity), 255);
        glowRGB[0] = std::min((int)(glow.bgra[0] * glow_intensity), 255);
#endif
        for (int i = 0; i < 3; i++) {
            color[i] = std::min<float>(5 + color[i] * (diffuse + .8f * specular) + glowRGB[i], 255);
        }
        return false;
    }
};

int main(int argc, char** argv) {

    TGAImage image(width, height, TGAImage::RGB);
    zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) zbuffer[i] = ZBUFFER_MIN;
    light = light.normalize();
    mProj = proj(-1.f / (eye - center).norm());

    Shader shader;
    shader.uniform_M = mProj * mMV;
    shader.uniform_MIT = (mProj * mMV).invert_transpose();

    for (int fileindex = 0; fileindex < file_count; fileindex++) {
        Model* model = new Model(files[fileindex]);
        current_model = model;
        
        for (int i = 0; i < model->nfaces(); i++) {
            std::vector<int> face = model->face(i);
            Vec4f points[3];
            for (int j = 0; j < 3; j++) {
                points[j] = shader.vertex(i, j);
            }
            triangle(points, &shader, zbuffer, image);
        }

        delete model;
    }

    //printArr(zbuffer, width, height);

    if (edge_cellshading) {
        const int edge_width = std::max(width / 1000, 2);

        TGAImage aoimage(image.get_width(), image.get_height(), TGAImage::RGB);
#pragma omp parallel for
        for (int i = 0; i < image.get_width(); i++) {
            for (int j = 0; j < image.get_height(); j++) {

                TGAColor pixel = image.get(i, j);
                const float pixel_depth = zbuffer[j * width + i];
                if (pixel_depth == ZBUFFER_MIN) {
                    aoimage.set(i, j, TGAColor(50, 50, 50));
                    continue;
                }

                int istart = -edge_width;
                int iend = edge_width;
                int jstart = -edge_width;
                int jend = edge_width;
                bool isEdge = false;

                for (int di = istart; di < iend; di++) {
                    for (int dj = jstart; dj < jend; dj++) {
                        int x = i + di;
                        if (x < 0 || x >= image.get_width()) continue;
                        int y = j + dj;
                        if (y < 0 || y >= image.get_height()) continue;
                        if (di == 0 && dj == 0) continue;
                        TGAColor neighbour = image.get(x, y);
                        if (zbuffer[y * width + x] == ZBUFFER_MIN) {
                            isEdge = true;
                            break;
                        }
                        const float neighbor_depth = zbuffer[y * width + x];
                        float distance = std::abs(neighbor_depth - pixel_depth);
                        if (distance > 0.4f && neighbor_depth > pixel_depth) {
                            isEdge = true;
                            break;
                        }
                    }
                }

                aoimage.set(i, j, isEdge ? TGAColor(0, 0, 0) : pixel);
            }
        }
        aoimage.write_tga_file("ecs_output.tga");
    }
    
    if (dof) {

        TGAImage aoimage(image.get_width(), image.get_height(), TGAImage::RGB);
        #pragma omp parallel for
        for (int i = 0; i < image.get_width(); i++) {
            for (int j = 0; j < image.get_height(); j++) {
                float pdepth = zbuffer[j * width + i];
                int ddepth = (int)(std::abs(focus - pdepth) * focus_strength);
                int blur_strength = std::min(std::max(ddepth, 0), 15);

                TGAColor pixel = image.get(i, j);
                float r = pixel.bgra[2];
                float g = pixel.bgra[1];
                float b = pixel.bgra[0];
                if (zbuffer[j * width + i] == ZBUFFER_MIN) continue;

                float pixelsweight = 1;

                int istart = -blur_strength;
                int iend = blur_strength;
                int jstart = -blur_strength;
                int jend = blur_strength;

                for (int di = istart; di < iend; di++) {
                    for (int dj = jstart; dj < jend; dj++) {
                        int x = i + di;
                        if (x < 0 || x >= image.get_width()) continue;
                        int y = j + dj;
                        if (y < 0 || y >= image.get_height()) continue;
                        if (di == 0 && dj == 0) continue;
                        TGAColor neighbour = image.get(x, y);
                        float distance = std::sqrt(std::pow((float)x-i, 2) + std::pow((float)y-j, 2));
                        float pixelweight = 1.f - (1.f / (distance + 1));
                        r += (float)neighbour.bgra[2] * pixelweight;
                        g += (float)neighbour.bgra[1] * pixelweight;
                        b += (float)neighbour.bgra[0] * pixelweight;
                        pixelsweight += pixelweight;
                    }
                }

                pixel.bgra[2] = (r / pixelsweight) * std::pow(0.92f, std::pow((float)ddepth, 1.2f));
                pixel.bgra[1] = (g / pixelsweight) * std::pow(0.92f, std::pow((float)ddepth, 1.2f));
                pixel.bgra[0] = (b / pixelsweight) * std::pow(0.92f, std::pow((float)ddepth, 1.2f));
                aoimage.set(i, j, pixel);
            }
        }
        aoimage.write_tga_file("dof_output.tga");
    }

    if (aa) {
        TGAImage aaimage(image.get_width(), image.get_height(), TGAImage::RGB);

        int const samplesize = 1;
#pragma omp
        for (int i = 0; i < image.get_width(); i++) {
            for (int j = 0; j < image.get_height(); j++) {
                TGAColor pixel = image.get(i, j);
                int r = 0;
                int g = 0;
                int b = 0;
                int pixelcount = 0;

                int istart = -samplesize;
                int iend = samplesize;
                int jstart = -samplesize;
                int jend = samplesize;

                for (int di = istart; di < iend; di++) {
                    for (int dj = jstart; dj < jend; dj++) {
                        int x = i + di;
                        if (x < 0 || x >= image.get_width()) continue;
                        int y = j + dj;
                        if (y < 0 || y >= image.get_height()) continue;
                        TGAColor neighbour = image.get(i + di, j + dj);
                        r += neighbour.bgra[2];
                        g += neighbour.bgra[1];
                        b += neighbour.bgra[0];
                        pixelcount++;
                    }
                }

                pixel.bgra[2] = r / pixelcount;
                pixel.bgra[1] = g / pixelcount;
                pixel.bgra[0] = b / pixelcount;
                aaimage.set(i, j, pixel);
            }
        }
        aaimage.write_tga_file("aa_output.tga");
    }

    //image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    return 0;
}
