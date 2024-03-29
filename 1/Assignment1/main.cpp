#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
	Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

	Eigen::Matrix4f translate;
	translate << 1, 0, 0, -eye_pos[0],
		0, 1, 0, -eye_pos[1],
		0, 0, 1, -eye_pos[2],
		0, 0, 0, 1;

	view = translate * view;

	return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
	Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

	// TODO: Implement this function
	// Create the model matrix for rotating the triangle around the Z axis.
	// Then return it.

	Eigen::Matrix4f rotation;
	float radian = rotation_angle / 180.f * MY_PI;
	rotation << cosf(radian), -sinf(radian), 0, 0,
		sinf(radian), cosf(radian), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1;

	model = rotation * model;

	return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
									  float zNear, float zFar)
{
	Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

	// TODO: Implement this function
	// Create the projection matrix for the given parameters.
	// Then return it.

	Eigen::Matrix4f perspective;
	perspective << zNear, 0, 0, 0,
		0, zNear, 0, 0,
		0, 0, zNear + zFar, -zNear * zFar,
		0, 0, 1, 0;

	float fov_radian = eye_fov / 180.f * MY_PI;
	float left, right, top, buttom;
	top = fabs(zNear) * tanf(fov_radian / 2);
	buttom = -top;
	right = top * aspect_ratio;
	left = -right;

	Eigen::Matrix4f ortho_translate, ortho_scale;
	ortho_translate << 1, 0, 0, -(right + left) / 2,
		0, 1, 0, -(top + buttom) / 2,
		0, 0, 1, -(zNear + zFar) / 2,
		0, 0, 0, 1;
	ortho_scale << 2 / (right - left), 0, 0, 0,
		0, 2 / (top - buttom), 0, 0,
		0, 0, 2 / (zNear - zFar), 0,
		0, 0, 0, 1;
	Eigen::Matrix4f orthographic = ortho_scale * ortho_translate;

	projection = orthographic * perspective;

	return projection;
}

Eigen::Matrix4f get_rotation(Vector3f axis, float angle)
{

	Eigen::Matrix4f rotation = Eigen::Matrix4f::Identity();

	// TODO: advanced task in Assignment1
	// this function returns a rotation matrix that
	// rotates a certain angle around an arbitrary axis crossing the origin.
	// Implemented by using the Rodrigues' Rotation Formula

	float radian = angle / 180.f * MY_PI;
	Eigen::Matrix3f N_matrix, Rodrigues_matrix;
	N_matrix << 0, -axis.z(), axis.y(),
		axis.z(), 0, -axis.x(),
		-axis.y(), axis.x(), 0;

	Rodrigues_matrix = cosf(radian) * Eigen::Matrix3f::Identity() + (1 - cosf(radian)) * axis * axis.transpose() + sinf(radian) * N_matrix;
	rotation << Rodrigues_matrix(0, 0), Rodrigues_matrix(0, 1), Rodrigues_matrix(0, 2), 0,
		Rodrigues_matrix(1, 0), Rodrigues_matrix(1, 1), Rodrigues_matrix(1, 2), 0,
		Rodrigues_matrix(2, 0), Rodrigues_matrix(2, 1), Rodrigues_matrix(2, 2), 0,
		0, 0, 0, 1;

	return rotation;
}

int main(int argc, const char **argv)
{
	float angle = 0;
	bool command_line = false;
	std::string filename = "output.png";

	if (argc >= 3)
	{
		command_line = true;
		angle = std::stof(argv[2]); // -r by default
		if (argc == 4)
		{
			filename = std::string(argv[3]);
		}
	}

	rst::rasterizer r(700, 700);

	Eigen::Vector3f eye_pos = {0, 0, 5};

	std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

	std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

	auto pos_id = r.load_positions(pos);
	auto ind_id = r.load_indices(ind);

	int key = 0;
	int frame_count = 0;

	if (command_line)
	{
		r.clear(rst::Buffers::Color | rst::Buffers::Depth);

		r.set_model(get_model_matrix(angle));
		r.set_view(get_view_matrix(eye_pos));
		r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

		r.draw(pos_id, ind_id, rst::Primitive::Triangle);
		cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
		image.convertTo(image, CV_8UC3, 1.0f);

		cv::imwrite(filename, image);

		return 0;
	}

	while (key != 27)
	{
		r.clear(rst::Buffers::Color | rst::Buffers::Depth);

		r.set_model(get_model_matrix(angle));
		r.set_view(get_view_matrix(eye_pos));
		r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

		r.draw(pos_id, ind_id, rst::Primitive::Triangle);

		cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
		image.convertTo(image, CV_8UC3, 1.0f);
		cv::imshow("image", image);
		key = cv::waitKey(10);

		std::cout << "frame count: " << frame_count++ << '\n';

		if (key == 'a')
		{
			angle += 10;
		}
		else if (key == 'd')
		{
			angle -= 10;
		}
	}

	return 0;
}
