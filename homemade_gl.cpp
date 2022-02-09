#include "homemade_gl.h"

Vec2f V3f2V2f(Vec3f v) {
    return Vec2f(v.x, v.y);
}

Vec2f V4f2V2f(Vec4f v) {
    return Vec2f(v[0], v[1]);
}

Matrix proj(float f) {
    Matrix res = Matrix::identity();
    res[4][3] = -1/f;
    return res;
}

Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity();
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = depth / 2.f;

    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = depth / 2.f;
    return m;
}

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    Matrix Minv = Matrix::identity();
    Matrix Tr = Matrix::identity();
    for (int i = 0; i < 3; i++) {
        Minv[0][i] = x[i];
        Minv[1][i] = y[i];
        Minv[2][i] = z[i];
        Tr[i][3] = -center[i];
    }
    return Minv * Tr;
}

Vec3f barycentric(Vec2f p0, Vec2f p1, Vec2f p2, Vec2f point) {
    Vec2f v0 = p1 - p0;
    Vec2f v1 = p2 - p0;
    Vec2f v2 = point - p0;

    float d = v0.x * v1.y - v1.x * v0.y;
    Vec3f bary;
    bary.y = (v2.x * v1.y - v1.x * v2.y) / d;
    bary.z = (v0.x * v2.y - v2.x * v0.y) / d;
    bary.x = 1 - bary.y - bary.z;
    return bary;
}

void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
    bool swapped = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        swapped = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;

    for (int x = x0; x <= x1; x++) {
        if (swapped) {
            image.set(y, x, color);
        }
        else {
            image.set(x, y, color);
        }
        error2 += derror2;
        if (error2 > dx) {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

void triangle(Vec4f* points, IShader* shader, float* zbuffer, TGAImage& image) {

    Vec2f pts[3] = { V4f2V2f(points[0]), V4f2V2f(points[1]), V4f2V2f(points[2]) };

    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f bound(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) { // For each point of the triangle
        for (int j = 0; j < 2; j++) { // Foreach dimension
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(bound[j], std::max(bboxmax[j], pts[i][j]));
        }
    }

    int bimi = bboxmin.x;
    int bima = bboxmax.x;

    for (int i = bboxmin.x; i < bboxmax.x; i++) {
        for (int j = bboxmin.y; j < bboxmax.y; j++) {
            Vec3f bary = barycentric(pts[0], pts[1], pts[2], Vec2f(i, j));
            if (bary.x < 0 || bary.y < 0 || bary.z < 0) continue;
            float z = 0;
            for (int k = 0; k < 3; k++) {
                z += points[k][2] * bary[k];
            }
            z /= 10;
            if (zbuffer[i + j * width] < z) {
                TGAColor color;
                bool discard = shader->fragment(bary, color);
                if (!discard) {
                    zbuffer[i + j * width] = z;
                    image.set(i, j, color);
                }
            }
        }
    }
}