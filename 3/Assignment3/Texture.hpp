//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <eigen3/Eigen/Eigen>
#include <opencv2/opencv.hpp>
#include <functional>

class Texture
{
private:
    cv::Mat image_data;

public:
    Texture(const std::string &name)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
    }

    int width, height;

    Eigen::Vector3f getColor(float u, float v)
    {
        auto u_img = u * width;
        auto v_img = (1 - v) * height;
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }

    Eigen::Vector3f getColorBilinear(float u, float v)
    {
        auto u_img = u * width;
        auto v_img = (1 - v) * height;

        float u_min = std::floor(u_img);
        float u_max = std::min(static_cast<float>(width), std::ceil(u_img));
        float v_min = std::floor(v_img);
        float v_max = std::min(static_cast<float>(height), std::ceil(v_img));

        cv::Vec3b u00 = image_data.at<cv::Vec3b>(v_max, u_min);
        cv::Vec3b u10 = image_data.at<cv::Vec3b>(v_max, u_max);
        cv::Vec3b u01 = image_data.at<cv::Vec3b>(v_min, u_min);
        cv::Vec3b u11 = image_data.at<cv::Vec3b>(v_min, u_max);
        std::function<cv::Vec3b(float, cv::Vec3b, cv::Vec3b)> lerp = [](float x, cv::Vec3b v0, cv::Vec3b v1)
        {
            return v0 + x * (v1 - v0);
        };

        float s = u_img - u_min;
        float t = v_img - v_max;
        auto u0 = lerp(s, u00, u10);
        auto u1 = lerp(s, u01, u11);
        auto color = lerp(t, u0, u1);

        return Eigen::Vector3f(color[0], color[1], color[2]);
    }
};
#endif //RASTERIZER_TEXTURE_H
