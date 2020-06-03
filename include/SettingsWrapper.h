#pragma once
#include <string>
#include <sstream>
#include <rapidjson/document.h>
#include <fstream>
#include <vector>

using std::string;
using std::stringstream;
using std::ifstream;
using rapidjson::Document;
using rapidjson::Value;


class SettingsEntryCommon;
namespace SettingsWrapperList {
	extern std::vector<SettingsEntryCommon*> all;
};

class SettingsEntryCommon {
public:
	string name;

	SettingsEntryCommon(const std::string& name) : name(name) {
		SettingsWrapperList::all.push_back(this);
	}

	virtual bool validateExistance(Document &d) {
		return d.HasMember(name.c_str());
	}
	virtual bool validateType(Document &d) = 0;
	virtual void addMember(Document &d) = 0;
	virtual void loadData(Document &d) = 0;
};

class SettingsEntryBool : public SettingsEntryCommon {
public:
	bool data;

	SettingsEntryBool(const std::string& name) : SettingsEntryCommon(name) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsBool();
	}

	virtual void addMember(Document &d) {

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

class SettingsEntryInt : public SettingsEntryCommon {
public:
	int data;

	SettingsEntryInt(const std::string& name) : SettingsEntryCommon(name) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsInt();
	}

	virtual void addMember(Document &d) {
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

class SettingsEntryUInt : public SettingsEntryCommon {
public:
	uint32_t data;

	SettingsEntryUInt(const std::string& name) : SettingsEntryCommon(name) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsUint();
	}

	virtual void addMember(Document &d) {
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

class SettingsEntryUInt8 : public SettingsEntryCommon {
public:
	uint8_t data;

	SettingsEntryUInt8(const std::string& name) : SettingsEntryCommon(name) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsUint();
	}

	virtual void addMember(Document &d) {
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

class SettingsEntryDouble : public SettingsEntryCommon {
public:
	double data;

	SettingsEntryDouble(const std::string& name) : SettingsEntryCommon(name) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsDouble();
	}

	virtual void addMember(Document &d) {
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

class SettingsEntryString : public SettingsEntryCommon {
public:
	string data;

	SettingsEntryString(const std::string& name) : SettingsEntryCommon(name) {}

	virtual bool validateType(Document &d) {
		return d[name.c_str()].IsString();
	}

	virtual void addMember(Document &d) {
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
	SettingsEntryBool debug{ "debug" };

	SettingsEntryInt com_timeout{ "com_timeout" };
	SettingsEntryInt com_baud{ "com_baud" };
	SettingsEntryInt com_port{ "com_port" };

	SettingsEntryString camera{ "camera" };

	bool socket_enable;
	SettingsEntryInt socket_port{ "socket_port" };

	SettingsEntryDouble sensor_width{ "sensor_width" };
	SettingsEntryDouble sensor_height{ "sensor_height" };
	SettingsEntryDouble focal_length_min{ "focal_length_min" };
	SettingsEntryDouble focal_length_max{ "focal_length_max" };
	SettingsEntryDouble principal_x{ "principal_x" };
	SettingsEntryDouble principal_y{ "principal_y" };
	SettingsEntryUInt imW{ "imW" };
	SettingsEntryUInt imH{ "imH" };
	SettingsEntryDouble image_resize_factor{ "image_resize_factor" };
	SettingsEntryUInt8 thresh_red{ "thresh_red" };
	SettingsEntryUInt8 thresh_s{ "thresh_s" };
	SettingsEntryUInt8 thresh_green{ "thresh_green" };

	SettingsEntryDouble motor_pan_factor{ "motor_pan_factor" };
	SettingsEntryDouble motor_pan_min{ "motor_pan_min" };
	SettingsEntryDouble motor_pan_max{ "motor_pan_max" };
	SettingsEntryDouble motor_pan_forward{ "motor_pan_forward" };
	SettingsEntryDouble motor_tilt_factor{ "motor_tilt_factor" };
	SettingsEntryDouble motor_tilt_min{ "motor_tilt_min" };
	SettingsEntryDouble motor_tilt_max{ "motor_tilt_max" };
	SettingsEntryDouble motor_tilt_forward{ "motor_tilt_forward" };
	SettingsEntryUInt motor_buffer_depth{ "motor_buffer_depth" };

	SettingsEntryBool show_frame_rgb{ "show_frame_rgb" };
	SettingsEntryBool show_frame_mask{ "show_frame_mask" };
	SettingsEntryBool show_frame_track{ "show_frame_track" };
	SettingsEntryBool print_coordinates{ "print_coordinates" };
	SettingsEntryBool print_rotation{ "print_rotation" };
	SettingsEntryBool print_info{ "print_info" };

	SettingsEntryString save_directory{ "save_directory" };

private:
	void verifyExistance(Document& d) {
		for (auto& v : SettingsWrapperList::all) {
			v->validateExistance(d);
		}
	}

	void verifyType(Document& d) {
		for (auto& v : SettingsWrapperList::all) {
			v->validateType(d);
		}
	}

	void loadValues(Document& d) {
		for (auto& v : SettingsWrapperList::all) {
			v->loadData(d);
		}
	}
public:
	SettingsWrapper(string fileName) {
		loadSettings(fileName);
	}

	SettingsWrapper() = default;

	void loadSettings(const string& fileName) {
		std::ifstream fileStream(fileName, std::ifstream::in);
		if (!fileStream.is_open()) {
			throw "File was not opened or not found.";
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
			throw "File was not opened or not found.";
		}
	}
};