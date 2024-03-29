#pragma once
#include <SettingsWrapper.h>
#include <vector>

namespace Network {
	void useSettings(SettingsWrapper &sw);
	int startServer();
	int acceptConnection();
	int sendData(const char* data, int len);
	std::vector<char> recvData();
	int recvData(char* data, int len);
	int getBytesReady();
	template<typename T> int sendData(const std::vector<T>& data) {
		int bytes = (int) data.size() * (int) sizeof(T);
		return sendData((char*)data.data(), bytes);
	}
	template<typename T> int sendData(const T& data) {
		int32_t bytes = (int32_t) sizeof(data);
		return sendData((const char*) &data, bytes);
	}
	template<typename T> std::vector<T> recvData() {
		int nBytes = getBytesReady();
		int extraElements = nBytes % (int) sizeof(T);
		if (extraElements) {
			std::cerr << "Received " << extraElements << "too many bytes to fit into an integer multiple of"
				 << type_info::name(T) << ". Rounding down to nearest integer multiple." << std::endl;
		}

		std::vector<T> returner((nBytes - extraElements)/sizeof(T));
		int nBytes = recvData(returner.data(), nBytes - extraElements);

		return returner;
	}
	int disconnectClient();
	int shutdownServer();
}