#pragma once
#include <string>
#include <sstream>
#include <rapidjson/document.h>
#include <fstream>

using std::string;
using std::stringstream;
using std::ifstream;
using rapidjson::Document;

class SettingsWrapper
{
public:
	bool debug;

	int com_timeout;
	int com_baud;
	int com_port;

	string camera;

	bool socket_enable;
	int socket_port;

	double sensor_width;
	double sensor_height;
	double focal_length_min;
	double focal_length_max;
	double image_resize_factor;
	uint8_t thresh_red;
	uint8_t thresh_s;

	double motor_pan_factor;
	double motor_pan_min;
	double motor_pan_max;
	double motor_tilt_factor;
	double motor_tilt_min;
	double motor_tilt_max;

	bool show_frame_rgb;
	bool show_frame_mask;
	bool show_frame_track;
	bool print_coordinates;
	bool print_rotation;
	bool print_info;

	Document d;

	void verifyExistance() {
		assert(d.HasMember("debug"));

		assert(d.HasMember("com_timeout"));
		assert(d.HasMember("com_baud"));
		assert(d.HasMember("com_port"));

		assert(d.HasMember("camera"));

		assert(d.HasMember("socket_port"));

		assert(d.HasMember("sensor_width"));
		assert(d.HasMember("sensor_height"));
		assert(d.HasMember("focal_length_min"));
		assert(d.HasMember("focal_length_max"));
		assert(d.HasMember("image_resize_factor"));
		assert(d.HasMember("thresh_red"));
		assert(d.HasMember("thresh_s"));

		assert(d.HasMember("motor_pan_factor"));
		assert(d.HasMember("motor_pan_min"));
		assert(d.HasMember("motor_pan_max"));
		assert(d.HasMember("motor_tilt_factor"));
		assert(d.HasMember("motor_tilt_min"));
		assert(d.HasMember("motor_tilt_max"));


		assert(d.HasMember("show_frame_rgb"));
		assert(d.HasMember("show_frame_mask"));
		assert(d.HasMember("show_frame_track"));
		assert(d.HasMember("print_coordinates"));
		assert(d.HasMember("print_rotation"));
		assert(d.HasMember("print_info"));
	}

	void verifyType() {
		assert(d["debug"].IsBool());

		assert(d["com_timeout"].IsInt());
		assert(d["com_baud"].IsInt() || d["com_baud"].IsNull());
		assert(d["com_port"].IsInt());

		assert(d["camera"].IsString());

		assert(d["socket_port"].IsInt() || d["socket_port"].IsNull());

		assert(d["sensor_width"].IsDouble());
		assert(d["sensor_height"].IsDouble());
		assert(d["focal_length_min"].IsDouble());
		assert(d["focal_length_max"].IsDouble());
		assert(d["image_resize_factor"].IsDouble());
		assert(d["thresh_red"].IsInt());
		assert(d["thresh_s"].IsInt());

		assert(d["motor_pan_factor"].IsDouble());
		assert(d["motor_pan_min"].IsDouble());
		assert(d["motor_pan_max"].IsDouble());
		assert(d["motor_tilt_factor"].IsDouble());
		assert(d["motor_tilt_min"].IsDouble());
		assert(d["motor_tilt_max"].IsDouble());

		assert(d["show_frame_rgb"].IsBool());
		assert(d["show_frame_mask"].IsBool());
		assert(d["show_frame_track"].IsBool());
		assert(d["print_coordinates"].IsBool());
		assert(d["print_rotation"].IsBool());
		assert(d["print_info"].IsBool());
	}

	void loadValues() {
		debug = d["debug"].GetBool();

		com_timeout = d["com_timeout"].GetInt();
		com_baud = d["com_baud"].GetInt();
		com_port = d["com_port"].GetInt();

		camera = d["camera"].GetString();

		socket_enable = !d["socket_port"].IsNull();
		if (socket_enable)
			socket_port = d["socket_port"].GetInt();

		sensor_width = d["sensor_width"].GetDouble();
		sensor_height = d["sensor_height"].GetDouble();
		focal_length_min = d["focal_length_min"].GetDouble();
		focal_length_max = d["focal_length_max"].GetDouble();
		image_resize_factor = d["image_resize_factor"].GetDouble();
		thresh_red = d["thresh_red"].GetInt();
		thresh_s = d["thresh_s"].GetInt();

		motor_pan_factor = d["motor_pan_factor"].GetDouble();
		motor_pan_min = d["motor_pan_min"].GetDouble();
		motor_pan_max = d["motor_pan_max"].GetDouble();
		motor_tilt_factor = d["motor_tilt_factor"].GetDouble();
		motor_tilt_min = d["motor_tilt_min"].GetDouble();
		motor_tilt_max = d["motor_tilt_max"].GetDouble();

		show_frame_rgb = d["show_frame_rgb"].GetBool();
		show_frame_mask = d["show_frame_mask"].GetBool();
		show_frame_track = d["show_frame_track"].GetBool();
		print_coordinates = d["print_coordinates"].GetBool();
		print_rotation = d["print_rotation"].GetBool();
		print_info = d["print_info"].GetBool();
	}

	SettingsWrapper(string fileName) {
		std::ifstream fileStream(fileName, std::ifstream::in);
		if (!fileStream.is_open()) {
			throw "File was not opened or not found.";
		}
		stringstream loadedFile;
		loadedFile << fileStream.rdbuf();

		d.Parse(loadedFile.str().data());
		verifyExistance();
		verifyType();
		loadValues();
	}
};

