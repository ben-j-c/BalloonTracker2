#include "PanTiltControl.h"

static SettingsWrapper& sw;


void PTC::useSettings(SettingsWrapper& wrap) {
	sw = wrap;
}