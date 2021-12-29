#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata)
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4)
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
                  << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }
}

void anti_aliasing(const cv::Point2f &point, cv::Mat &window, int bgr_idx)
{
    float x_min = std::floor(point.x);
    float x_max = std::ceil(point.x);
    float y_min = std::floor(point.y);
    float y_max = std::ceil(point.y);

    std::vector<cv::Point2f> neighbor_points(4);
    neighbor_points[0] = {x_min, y_min};
    neighbor_points[1] = {x_min, y_max};
    neighbor_points[2] = {x_max, y_min};
    neighbor_points[3] = {x_max, y_max};
    for (const auto &n_point : neighbor_points)
    {
        auto p = point - n_point;
        float distance = p.dot(p);
        if (window.at<cv::Vec3b>(n_point)[bgr_idx] < static_cast<uchar>(255.f * (2.f - distance) / 2.f))
        {
            window.at<cv::Vec3b>(n_point)[bgr_idx] = static_cast<uchar>(255.f * (2.f - distance) / 2.f);
        }
    }
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window)
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001)
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                     3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;

        // window.at<cv::Vec3b>(point.y, point.x)[2] = 255; //BGR
        anti_aliasing(point, window, 2);
    }
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t)
{
    // TODO: Implement de Casteljau's algorithm
    int sz = control_points.size();
    std::vector<cv::Point2f> iter_points(control_points);

    while (sz > 1)
    {
        cv::Point2f beg_point = iter_points[0];
        for (int i = 0; i < sz - 1; ++i)
        {
            iter_points[i] = (1 - t) * beg_point + t * iter_points[i + 1];
            beg_point = iter_points[i + 1];
        }
        --sz;
    }

    return cv::Point2f(iter_points[0]);
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window)
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps,
    // and call de Casteljau's recursive Bezier algorithm.

    for (float t = 0.f; t <= 1.f; t += 0.001f)
    {
        // window.at<cv::Vec3b>(recursive_bezier(control_points, t))[1] = 255; //BGR
        anti_aliasing(recursive_bezier(control_points, t), window, 1);
    }
}

int main()
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27)
    {
        for (auto &point : control_points)
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4)
        {
            naive_bezier(control_points, window);
            bezier(control_points, window);

            cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

    return 0;
}