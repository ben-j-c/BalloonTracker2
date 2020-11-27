#pragma once
#include <string>
#include <sstream>
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <fstream>
#include <vector>

using std::string;
using std::stringstream;
using std::ifstream;
using rapidjson::Document;
using rapidjson::Value;

#define SETTINGSENTRY(x, name) SettingsEntry_##x name{ #name, all };

class SettingsEntryCommon {
public:
	string name;
	string type;

	SettingsEntryCommon(const std::string& name, const std::string& type, std::vector<SettingsEntryCommon*>& vec) : name(name), type(type) {
		vec.push_back(this);
	}

	virtual bool validateExistance(Document &d) {
		return d.HasMember(name.c_str());
	}
	virtual bool validateType(Document &d) = 0;
	virtual void addMember(Document &d) = 0;
	virtual void loadData(Document &d) = 0;
};

class SettingsEntry_bool : public SettingsEntryCommon {
public:
	bool data;

	SettingsEntry_bool(const std::string& name, std::vector<SettingsEntryCommon*>& vec) : SettingsEntryCommon(name, "bool", vec) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsBool();
	}

	virtual void addMember(Document &d) {
		d.AddMember(rapidjson::StringRef(name), Value(data), d.GetAllocator());
	}

	virtual void loadData(Document &d) {
		data = d[name.c_str()].GetBool();
	}

	operator bool() {
		return data;
	}

	bool* operator&() {
		return &data;
	}

	bool& operator=(const bool& data) {
		return this->data = data;
	}
};

class SettingsEntry_int : public SettingsEntryCommon {
public:
	int data;

	SettingsEntry_int(const std::string& name, std::vector<SettingsEntryCommon*>& vec) : SettingsEntryCommon(name, "int", vec) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsInt();
	}

	virtual void addMember(Document &d) {
		d.AddMember(rapidjson::StringRef(name), Value(data), d.GetAllocator());
	}

	virtual void loadData(Document &d) {
		data = d[name.c_str()].GetInt();
	}

	operator int() {
		return data;
	}

	int* operator&() {
		return &data;
	}

	int& operator=(const int& data) {
		return this->data = data;
	}
};

class SettingsEntry_uint32_t : public SettingsEntryCommon {
public:
	uint32_t data;

	SettingsEntry_uint32_t(const std::string& name, std::vector<SettingsEntryCommon*>& vec) : SettingsEntryCommon(name, "uint32_t", vec) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsUint();
	}

	virtual void addMember(Document &d) {
		d.AddMember(rapidjson::StringRef(name), Value(data), d.GetAllocator());
	}

	virtual void loadData(Document &d) {
		data = d[name.c_str()].GetUint();
	}

	operator uint32_t() {
		return data;
	}

	uint32_t* operator&() {
		return &data;
	}

	uint32_t& operator=(const uint32_t& data) {
		return this->data = data;
	}
};

class SettingsEntry_uint8_t : public SettingsEntryCommon {
public:
	uint8_t data;

	SettingsEntry_uint8_t(const std::string& name, std::vector<SettingsEntryCommon*>& vec) : SettingsEntryCommon(name, "uint8_t", vec) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsUint();
	}

	virtual void addMember(Document &d) {
		d.AddMember(rapidjson::StringRef(name), Value(data), d.GetAllocator());
	}

	virtual void loadData(Document &d) {
		data = d[name.c_str()].GetUint();
	}

	operator uint32_t() {
		return data;
	}

	uint8_t* operator&() {
		return &data;
	}

	uint8_t& operator=(const uint8_t& data) {
		return this->data = data;
	}
};

class SettingsEntry_double : public SettingsEntryCommon {
public:
	double data;

	SettingsEntry_double(const std::string& name, std::vector<SettingsEntryCommon*>& vec) : SettingsEntryCommon(name, "double", vec) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsDouble();
	}

	virtual void addMember(Document &d) {
		d.AddMember(rapidjson::StringRef(name), Value(data), d.GetAllocator());
	}

	virtual void loadData(Document &d) {
		data = d[name.c_str()].GetDouble();
	}

	operator double() {
		return data;
	}

	double* operator&() {
		return &data;
	}

	double& operator=(const double& data) {
		return this->data = data;
	}
};

class SettingsEntry_string : public SettingsEntryCommon {
public:
	string data;

	SettingsEntry_string(const std::string& name, std::vector<SettingsEntryCommon*>& vec) : SettingsEntryCommon(name, "string", vec) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsString();
	}

	virtual void addMember(Document &d) {
		d.AddMember(rapidjson::StringRef(name), data, d.GetAllocator());
	}

	virtual void loadData(Document &d) {
		data = d[name.c_str()].GetString();
	}

	operator string() {
		return data;
	}

	string* operator&() {
		return &data;
	}

	string& operator=(const string& data) {
		return this->data = data;
	}
};

class SettingsWrapper {
public:
	std::vector<SettingsEntryCommon*> all;

	SETTINGSENTRY(bool, debug);

	SETTINGSENTRY(int, com_timeout);
	SETTINGSENTRY(int, com_port);
	SETTINGSENTRY(int, com_baud);

	SETTINGSENTRY(string, camera);
	SETTINGSENTRY(string, onvifEndpoint);
	SETTINGSENTRY(uint32_t, camera_framerate);

	SETTINGSENTRY(bool, socket_enable);
	SETTINGSENTRY(int, socket_port);

	SETTINGSENTRY(double, sensor_width);
	SETTINGSENTRY(double, sensor_height);
	SETTINGSENTRY(double, focal_length_min);
	SETTINGSENTRY(double, focal_length_max);
	SETTINGSENTRY(double, principal_x);
	SETTINGSENTRY(double, principal_y);
	SETTINGSENTRY(uint32_t, imW);
	SETTINGSENTRY(uint32_t, imH);
	SETTINGSENTRY(double, image_resize_factor);
	SETTINGSENTRY(uint8_t, thresh_red);
	SETTINGSENTRY(uint8_t, thresh_s);
	SETTINGSENTRY(uint8_t, thresh_green);

	SETTINGSENTRY(double, motor_pan_factor);
	SETTINGSENTRY(double, motor_pan_min);
	SETTINGSENTRY(double, motor_pan_max);
	SETTINGSENTRY(double, motor_pan_forward);
	SETTINGSENTRY(double, motor_tilt_factor);
	SETTINGSENTRY(double, motor_tilt_min);
	SETTINGSENTRY(double, motor_tilt_max);
	SETTINGSENTRY(double, motor_tilt_forward);
	SETTINGSENTRY(uint32_t, motor_buffer_depth);

	SETTINGSENTRY(bool, show_frame_rgb);
	SETTINGSENTRY(bool, show_frame_mask);
	SETTINGSENTRY(bool, show_frame_mask_s);
	SETTINGSENTRY(bool, show_frame_mask_r);
	SETTINGSENTRY(bool, show_frame_mask_g);
	SETTINGSENTRY(bool, show_frame_track);
	SETTINGSENTRY(bool, print_coordinates);
	SETTINGSENTRY(bool, print_rotation);
	SETTINGSENTRY(bool, print_info);

	SETTINGSENTRY(bool, report_motor_rotation);
	SETTINGSENTRY(bool, report_perceived_rotation);
	SETTINGSENTRY(bool, report_pose_estimation);
	SETTINGSENTRY(bool, report_altitude_estimation);
	SETTINGSENTRY(bool, report_frame_number);

	SETTINGSENTRY(string, save_directory);

private:
	void verifyExistance(Document& d) {
		for (auto& v : all) {
			if (!v->validateExistance(d)) {
				throw std::runtime_error("JSON setting missing: " + v->name);
			}
		}
	}

	void verifyType(Document& d) {
		for (auto& v : all) {
			if (!v->validateType(d)) {
				static const char* types[] =
				{ "Null", "False", "True", "Object", "Array", "String", "Number" };
				throw std::runtime_error("JSON entry \"" + v->name + "\" needs to be a " + v->type + ", but was found to be a " + types[d[v->name].GetType()]);
			}
		}
	}

	void loadValues(Document& d) {
		for (auto& v : all) {
			v->loadData(d);
		}
	}
public:
	SettingsWrapper(string fileName) {
		try {
			loadSettings(fileName);
		}
		catch (std::runtime_error e) {
			printf(e.what());
			throw e;
		}
	}

	SettingsWrapper() = default;

	void loadSettings(const string& fileName) {

		std::ifstream fileStream(fileName, std::ifstream::in);
		if (!fileStream.is_open()) {
			throw std::runtime_error("File was not opened or not found.");
		}
		stringstream loadedFile;
		loadedFile << fileStream.rdbuf();
		Document d;
		d.Parse(loadedFile.str().data());
		verifyExistance(d);
		verifyType(d);
		loadValues(d);
	}

	void saveSettings(const string& fileName) {
		std::ofstream fileStream(fileName, std::ofstream::out);
		if (!fileStream.is_open()) {
			throw std::runtime_error("File was not opened or not found.");
		}
		Document d;
		d.SetObject();
		for (auto& v : all) {
			v->addMember(d);
		}

		rapidjson::StringBuffer buf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
		d.Accept(writer);

		std::string s(buf.GetString());
		fileStream << s;
		fileStream.close();
	}
};