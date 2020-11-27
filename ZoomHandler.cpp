#include "ZoomHandler.h"
#include <curl/curl.h>

std::shared_ptr<CURLcode> curlInit =
std::shared_ptr<CURLcode>(
	new CURLcode(curl_global_init(CURL_GLOBAL_ALL)),
	[](CURLcode* c) {delete c; curl_global_cleanup(); });


const char* zoomLiteral =
"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"> \
<s:Body \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\
<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">\
<ProfileToken>PROFILE_000</ProfileToken>\
<Velocity><Zoom x=\"%f\" xmlns=\"http://www.onvif.org/ver10/schema\"/>\
</Velocity>\
</ContinuousMove>\
</s:Body>\
</s:Envelope>";

const char* stopLiteral =
"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">\
<s:Body \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\
<Stop xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">\
<ProfileToken>PROFILE_000</ProfileToken>\
<PanTilt>false</PanTilt>\
<Zoom>true</Zoom>\
</Stop>\
</s:Body>\
</s:Envelope>";


const char* stopHeaderLiteral = "Content-Type: application/soap+xml; charset=utf-8; action=\"http://www.onvif.org/ver20/ptz/wsdl/Stop\"";
const char* zoomHeaderLiteral = "Content-Type: application/soap+xml; charset=utf-8; action=\"http://www.onvif.org/ver20/ptz/wsdl/ContinuousMove\"";

void ZoomHandler::postData(const char* data, const char* endPoint, const char* head) {
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if (curl) {
		struct curl_slist* header = nullptr;
		header = curl_slist_append(header, head);
		header = curl_slist_append(header, "Accept-Encoding: deflate");


		curl_easy_setopt(curl, CURLOPT_URL, endPoint);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

		curl_easy_cleanup(curl);
	}
}

void ZoomHandler::zoom(float direction, const char* endPoint) {
	char buffer[1024] = {0};
	snprintf(buffer, 1024, zoomLiteral, direction);
	postData(buffer, endPoint, zoomHeaderLiteral);
}

void ZoomHandler::zoomIn() {
	if (!zoomingIn)
		zoom(1, endPoint.c_str());
	zoomingIn = true;
}

void ZoomHandler::zoomOut() {
	if(!zoomingOut)
		zoom(-1, endPoint.c_str());
	zoomingOut = true;
}

void ZoomHandler::stop() {
	if (zoomingIn || zoomingOut)
		postData(stopLiteral, endPoint.c_str(), stopHeaderLiteral);
	zoomingIn = false;
	zoomingOut = false;
}