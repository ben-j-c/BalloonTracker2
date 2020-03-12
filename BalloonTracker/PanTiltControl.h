#include <SettingsWrapper.h>
#include <iostream>
#include <string>

#pragma once
namespace PTC {
	void useSettings(SettingsWrapper& sw);
	void panCallback(int value, void*);
	void tiltCallback(int value, void*);
}

