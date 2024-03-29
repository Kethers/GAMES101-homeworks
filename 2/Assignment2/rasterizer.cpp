//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include <functional>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>

rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f &v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}

static bool insideTriangle(float x, float y, const Vector3f *_v)
{
    // TODO: Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]

    // use the corss product of (vector of triangle edge) and (vector of same starting point of the edge to the point)
    // if the sign of the z-value of three cross products are the same (all positive or all negative),
    // then the point is in the triangle, otherwise is not.
    Vector3f vec_v_to_P = {x - _v[2].x(), y - _v[2].y(), -_v[2].z()};
    Vector3f vec_v_to_v = {_v[0].x() - _v[2].x(), _v[0].y() - _v[2].y(), _v[0].z() - _v[2].z()};
    auto cross_z = vec_v_to_v.cross(vec_v_to_P).z();
    for (int i = 0; i < 2; ++i)
    {
        vec_v_to_P = {x - _v[i].x(), y - _v[i].y(), -_v[i].z()};
        vec_v_to_v = {_v[i + 1].x() - _v[i].x(), _v[i + 1].y() - _v[i].y(), _v[i + 1].z() - _v[i].z()};
        if (cross_z * vec_v_to_v.cross(vec_v_to_P).z() < 0)
        {
            return false;
        }
    }
    return true;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f *v)
{
    float c1 = (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y()) / (v[0].x() * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * v[0].y() + v[1].x() * v[2].y() - v[2].x() * v[1].y());
    float c2 = (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y()) / (v[1].x() * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * v[1].y() + v[2].x() * v[0].y() - v[0].x() * v[2].y());
    float c3 = (x * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * y + v[0].x() * v[1].y() - v[1].x() * v[0].y()) / (v[2].x() * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * v[2].y() + v[0].x() * v[1].y() - v[1].x() * v[0].y());
    return {c1, c2, c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto &buf = pos_buf[pos_buffer.pos_id];
    auto &ind = ind_buf[ind_buffer.ind_id];
    auto &col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto &i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
            mvp * to_vec4(buf[i[0]], 1.0f),
            mvp * to_vec4(buf[i[1]], 1.0f),
            mvp * to_vec4(buf[i[2]], 1.0f)};
        // Homogeneous division
        for (auto &vec : v)
        {
            vec /= vec.w();
        }
        // Viewport transformation
        for (auto &vert : v)
        {
            vert.x() = 0.5 * width * (vert.x() + 1.0);
            vert.y() = 0.5 * height * (vert.y() + 1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

// Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle &t)
{
    auto v = t.toVector4();

    // TODO: Find out the bounding box of current triangle.
    // iterate through the pixel and find if the current pixel is inside the triangle
    int xmax = INT_MIN, ymax = INT_MIN;
    int xmin = INT_MAX, ymin = INT_MAX;
    for (const auto &cord : v)
    {
        xmax = std::max(xmax, static_cast<int>(std::ceil(cord.x())));
        ymax = std::max(ymax, static_cast<int>(std::ceil(cord.y())));
        xmin = std::min(xmin, static_cast<int>(std::floor(cord.x())));
        ymin = std::min(ymin, static_cast<int>(std::floor(cord.y())));
    }

    std::function<float(float, float, Triangle)> depth_interpolate = [&](const float &x, const float &y, const Triangle &t)
    {
        auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
        float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
        float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
        // z_interpolated *= w_reciprocal;
        return z_interpolated * w_reciprocal;
    };

    bool MSAA = true;
    std::vector<Vector2f> pos = {{0.25, 0.25}, {0.25, 0.75}, {0.75, 0.25}, {0.75, 0.75}};

    for (int x = xmin; x <= xmax; ++x)
    {
        for (int y = ymin; y <= ymax; ++y)
        {
            if (MSAA)
            {
                int count = 0; // tear apart 1 pixel into 4 subpixels and record how many subpixels are in the triangle
                int eid = get_index(x, y) * 4;
                for (int i = 0; i < 4; ++i)
                {
                    float tmpx = x + pos[i][0], tmpy = y + pos[i][1];
                    if (insideTriangle(tmpx, tmpy, t.v))
                    {
                        float z_interpolated = depth_interpolate(tmpx, tmpy, t);
                        if (z_interpolated < super_depth_buf[eid + i])
                        {
                            super_depth_buf[eid + i] = z_interpolated;
                            super_frame_buf[eid + i] = t.getColor();
                            ++count; // record how many subpixels pass the depth test
                        }
                    }
                }
                if (count != 0)
                {
                    int depth_buf_index = get_index(x, y);
                    Vector3f point{(float)x, (float)y, 0.0f};
                    Vector3f color{(super_frame_buf[eid] + super_frame_buf[eid + 1] + super_frame_buf[eid + 2] + super_frame_buf[eid + 3]) / 4};
                    // convolute/average the color
                    set_pixel(point, color);
                }
            }
            else
            {
                if (insideTriangle(x + 0.5f, y + 0.5f, t.v))
                {
                    // If so, use the following code to get the interpolated z value.
                    // (encapsuled in the depth_interpolate lambda expression)

                    float z_interpolated = depth_interpolate(x + 0.5f, y + 0.5f, t);
                    int depth_buf_index = get_index(x, y);
                    if (z_interpolated < depth_buf[depth_buf_index])
                    {
                        depth_buf[depth_buf_index] = z_interpolated;

                        // TODO: set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
                        set_pixel(Vector3f(x, y, 1.f), t.getColor());
                    }
                }
            }
        }
    }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f &m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f &v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f &p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
        std::fill(super_frame_buf.begin(), super_frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
        std::fill(super_depth_buf.begin(), super_depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    super_frame_buf.resize(4 * w * h);
    super_depth_buf.resize(4 * w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height - 1 - y) * width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f &point, const Eigen::Vector3f &color)
{
    // old index: auto ind = point.y() + point.x() * width;
    auto ind = (height - 1 - point.y()) * width + point.x();
    frame_buf[ind] = color;
}

// clang-format on