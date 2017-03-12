#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

std::string homeDir =
#ifdef _WIN64
	getenv("USERPROFILE");
#elif _WIN32
   	getenv("USERPROFILE");
#elif __APPLE__
    getenv("HOME");
#elif __linux
	getenv("HOME");
#endif

std::string dolphinDumpPath =
#ifdef _WIN64
   	"\\My Documents\\Dolphin Emulator\\Dump\\ram.raw";
#elif _WIN32
   	"\\My Documents\\Dolphin Emulator\\Dump\\ram.raw";
#elif __APPLE__
    "/Library/Application Support/Dolphin/Dump/ram.raw";
#elif __linux
	"/.dolphin-emu/Dump/ram.raw";
#endif


std::vector< std::vector<std::string> > getHexDataFromDumpFile(std::string customFilename) {
    std::string filename = customFilename.length() > 0 ? customFilename : homeDir + dolphinDumpPath;
    std::ifstream ramDumpFile(filename.c_str(), std::ios::in | std::ios::binary);

	if (!ramDumpFile.is_open()) {
		throw "RAM dump could not be opened\n";
	}

	std::streampos fsize = 0;

    fsize = ramDumpFile.tellg();
    ramDumpFile.seekg(0, std::ios::end);
    fsize = ramDumpFile.tellg() - fsize;

	ramDumpFile.clear();
	ramDumpFile.seekg(0, std::ios::beg);


	char *buffer = new char[fsize];
	ramDumpFile.read(buffer, fsize);
	ramDumpFile.close();
	unsigned char *buf = (unsigned char*)buffer;


	std::vector< std::vector<std::string> > hexDataVector;
	int index = 0, i, j;
  	for (i=0; i<fsize; i+=16) {

		std::string hex = "";
		char hexArray[4];
    	for (j=0; j<16; j++)
      		if (i+j < fsize) {
				snprintf(hexArray, 4, "%02x", buf[i+j]);
				hex += hexArray;
			}

		std::string hexCharacterRepresentation = "";
		char representationArray[4];
    	for (j=0; j<16; j++)
      		if (i+j < fsize) {
				snprintf(representationArray, 2, "%c", isprint(buf[i+j]) ? buf[i+j] : '.');
				hexCharacterRepresentation += representationArray;
			}

		std::vector<std::string> hexData(2);
		hexData[0] = hex;
		hexData[1] = hexCharacterRepresentation;

		hexDataVector.push_back(hexData);
  	}

	delete[] buffer;

	return hexDataVector;
}


int main(int argc, char** argv) {
	std::string customFilename = "";
	if (argc > 1) {
		customFilename = argv[1];
	}

    std::cout << "\nDolphin Memory Comparator\n\n";

    std::cout << "Collect first memory dump:\n"
         << "1.) Place breakpoint in Dolphin on desired line\n"
         << "2.) Allow execution to halt on breakpoint\n"
         << "3.) Click 'Dump MRAM' button in 'Memory' tab to dump memory\n\n";

    std::cout << "Press RETURN key to process first memory dump for comparison...";

    std::cin.get();

	std::cout << "Collecting data from first memory dump...\n";
	std::vector< std::vector<std::string> > before = getHexDataFromDumpFile(customFilename);
	std::cout << "OK\n\n";

	std::cout << "Collect second memory dump:\n"
         << "1.) Advance program execution to desired instruction\n"
         << "2.) Click \'Dump MRAM\' button in \'Memory\' tab to dump memory again\n\n";

	std::cout << "Press RETURN key to process second memory dump for comparison...";

	std::cin.get();

	std::cout << "Collecting data from second memory dump...\n";
	std::vector< std::vector<std::string> > after = getHexDataFromDumpFile(customFilename);
	std::cout << "OK\n\n";

	std::cout << "Running comparison...\n";

	std::vector< std::vector<std::string> > differences;
	for (int i=0; i<before.size(); ++i) {
		std::string beforeHex = before.at(i).at(0);
		std::string afterHex = after.at(i).at(0);

		if (beforeHex != afterHex) {
			// Entire hex value is different, now find per bytes

			char address[8];
			snprintf(address, 8, "%07x", i*16);

			// First Byte
			std::string beforeFirstByte = beforeHex.substr(0, 8);
			std::string afterFirstByte = afterHex.substr(0, 8);
			if (beforeFirstByte != afterFirstByte) {
				char address[8];
				snprintf(address, 8, "%07x", (i*16) + 0);
				std::string addressStr = "8";
				addressStr += address;

				std::vector<std::string> differenceVector(5);
				differenceVector[0] = addressStr;
				differenceVector[1] = beforeFirstByte;
				differenceVector[2] = before.at(i).at(1).substr(0, 4);
				differenceVector[3] = afterFirstByte;
				differenceVector[4] = after.at(i).at(1).substr(0, 4);

				differences.push_back(differenceVector);
			}

			// Second byte
			std::string beforeSecondByte = beforeHex.substr(8, 8);
			std::string afterSecondByte = afterHex.substr(8, 8);
			if (beforeSecondByte != afterSecondByte) {
				char address[8];
				snprintf(address, 8, "%07x", (i*16) + 4);
				std::string addressStr = "8";
				addressStr += address;

				std::vector<std::string> differenceVector(5);
				differenceVector[0] = addressStr;
				differenceVector[1] = beforeSecondByte;
				differenceVector[2] = before.at(i).at(1).substr(4, 4);
				differenceVector[3] = afterSecondByte;
				differenceVector[4] = after.at(i).at(1).substr(4, 4);

				differences.push_back(differenceVector);
			}

			// Third byte
			std::string beforeThirdByte = beforeHex.substr(16, 8);
			std::string afterThirdByte = afterHex.substr(16, 8);
			if (beforeThirdByte != afterThirdByte) {
				char address[8];
				snprintf(address, 8, "%07x", (i*16) + 8);
				std::string addressStr = "8";
				addressStr += address;

				std::vector<std::string> differenceVector(5);
				differenceVector[0] = addressStr;
				differenceVector[1] = beforeThirdByte;
				differenceVector[2] = before.at(i).at(1).substr(8, 4);
				differenceVector[3] = afterThirdByte;
				differenceVector[4] = after.at(i).at(1).substr(8, 4);

				differences.push_back(differenceVector);
			}

			// Fourth byte
			std::string beforeFourthByte = beforeHex.substr(24, 8);
			std::string afterFourthByte = afterHex.substr(24, 8);
			if (beforeFourthByte != afterFourthByte) {
				char address[8];
				snprintf(address, 8, "%07x", (i*16) + 12);
				std::string addressStr = "8";
				addressStr += address;

				std::vector<std::string> differenceVector(5);
				differenceVector[0] = addressStr;
				differenceVector[1] = beforeFourthByte;
				differenceVector[2] = before.at(i).at(1).substr(12, 4);
				differenceVector[3] = afterFourthByte;
				differenceVector[4] = after.at(i).at(1).substr(12, 4);

				differences.push_back(differenceVector);
			}

		}
	}
	std::cout << "OK\n\n";

	if (differences.size() == 0)
		std::cout << "No differences found between memory dumps\n\n";
	else {
		std::cout << differences.size() << " differences found in the format:\n"
		          << "Address(Number in list)\n--------\nValue Before |ASCII|\nValue After  |ASCII|\n\n";
		for (int i=0; i<differences.size(); ++i) {
			std::cout << differences.at(i).at(0) << "(" << i+1 << ")\n--------\n"
					  << differences.at(i).at(1) << " |" << differences.at(i).at(2) << "|\n"
				      << differences.at(i).at(3) << " |" << differences.at(i).at(4) << "|\n\n";
		}
	}

}
