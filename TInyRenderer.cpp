// TInyRenderer.cpp : Defines the entry point for the application.
//

#include "cstdlib"
#include "iostream"
#include <cmath>
#include "model.h"
#include "homemade_gl.h"

Vec3f camera(0, 0, 3);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Matrix mVP = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
Matrix mProj = Matrix::identity();
Matrix mMV = lookat(eye, center, Vec3f(0, 1, 0));
Model* current_model = 0;
Vec3f light(-1, -0.3, 1);

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
        for (int i = 0; i < 3; i++) color[i] = std::min<float>(5 + color[i] * (diffuse + .4 * specular), 255);
        return false;
    }
};

int main(int argc, char** argv) {

    TGAImage image(width, height, TGAImage::RGB);
    float* zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) zbuffer[i] = INT_MIN;
    light = light.normalize();
    mProj = proj(-1.f / (eye - center).norm());

    Shader shader;
    shader.uniform_M = mProj * mMV;
    shader.uniform_MIT = (mProj * mMV).invert_transpose();

    const char* files[1] = { "obj/african_head/african_head.obj" }; int file_count = 1;
    // const char* files[3] = { "obj/african_head/african_head.obj", "obj/african_head/african_head_eye_outer.obj", "obj/african_head/african_head_eye_inner.obj" }; int file_count = 3;
    // const char* files[3] = { "obj/boggie/body.obj", "obj/boggie/head.obj", "obj/boggie/eyes.obj" }; int file_count = 3;
    // const char* files[1] = { "obj/diablo3_pose/diablo3_pose.obj" }; int file_count = 1;

    for (int fileindex = 0; fileindex < file_count; fileindex++) {
        Model* model = new Model(files[fileindex]);
        current_model = model;
        
        for (int i = 0; i < model->nfaces(); i++) {
            std::vector<int> face = model->face(i);
            Vec4f points[3];
            for (int j = 0; j < 3; j++) {
                Vec3f v = model->vert(face[j]);
                points[j] = shader.vertex(i, j);
            }
            triangle(points, &shader, zbuffer, image);
        }

        delete model;
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
