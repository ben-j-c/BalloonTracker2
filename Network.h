#pragma once
#include <SettingsWrapper.h>
#include <vector>

namespace Network {
	void useSettings(SettingsWrapper &sw);
	int startServer();
	int acceptConnection();
	int sendData(char* data, int len);
	std::vector<char> recvData();
	int recvData(char* data, int len);
	int getBytesReady();
	template<typename T> int sendData(std::vector<T>& data) {
		int bytes = data.size() * sizeof(T);
		sendData((char*)data.data(), bytes);
	}
	template<typename T> std::vector<T> recvData() {
		int nBytes = getBytesReady();
		int extraElements = nBytes % sizeof(T);
		if (extraElements) {
			std::cerr << "Received " << extraElements << "too many bytes to fit into an integer multiple of"
				 << type_info::name(T) << ". Rounding down to nearest integer multiple." << std::endl;
		}

		std::vector<T> returner((nBytes - extraElements)/sizeof(T));
		int nBytes = recvData(returner.data(), nBytes - extraElements);

		return returner;
	}
}