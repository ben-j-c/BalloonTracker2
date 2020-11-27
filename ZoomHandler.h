#pragma once
#include <string>

class ZoomHandler {
public:
	ZoomHandler(const std::string& endPoint) : endPoint(endPoint) {};
	void zoomIn();
	void zoomOut();
	void stop();
	bool zoomingIn = false, zoomingOut = false;
private:
	std::string endPoint;
	void postData(const char* data, const char* endPoint, const char* head);
	void zoom(float direction, const char* endPoint);
};

