#include "FileIO.h"
#include "debug/Debug.h"
#include "fstream"

std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file) {
		logWarning("could not open shader");
		return {};
	}

	auto fileLength{static_cast<size_t>(file.tellg())};

	std::vector<char> buf(fileLength);
	file.seekg(0);
	file.read(buf.data(), fileLength);
	file.close();

	return buf;
}
