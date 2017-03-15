#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

#include <sys/stat.h>
#include <pthread.h>

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
   	"\\My Documents\\Dolphin Emulator\\Dump\\";
#elif _WIN32
   	"\\My Documents\\Dolphin Emulator\\Dump\\";
#elif __APPLE__
    "/Library/Application Support/Dolphin/Dump/";
#elif __linux
	"/.dolphin-emu/Dump/";
#endif

struct hex_data {
  bool collectedBufferData;
  bool collectedHexEntries;

  std::string filepath;
  std::streampos fsize;
  char *buffer;
  std::vector< std::vector<std::string> > hexEntries;
  int rcGetHexEntriesThread;
} sramHexDataBefore, sramHexDataAfter, aramHexDataBefore, aramHexDataAfter;

struct hex_compare {
    hex_data *before;
    hex_data *after;
    std::vector< std::vector<std::string> > differences;
} sramHexCompare, aramHexCompare;

bool concurrency = true;


bool doesFileExist(std::string filepath) {
    struct stat buffer;
    return (stat (filepath.c_str(), &buffer) == 0);
}

void *collectHexDataFromBuffer(void *arg) {
    hex_data *data = (hex_data *)arg;

	unsigned char *buf = (unsigned char*)data->buffer;

	std::vector< std::vector<std::string> > hexDataVector;
	int index = 0, i, j;
  	for (i=0; i<data->fsize; i+=16) {

		std::string hex = "";
		char hexArray[4];
    	for (j=0; j<16; j++)
      		if (i+j < data->fsize) {
				snprintf(hexArray, 4, "%02x", buf[i+j]);
				hex += hexArray;
			}

		std::string hexCharacterRepresentation = "";
		char representationArray[4];
    	for (j=0; j<16; j++)
      		if (i+j < data->fsize) {
				snprintf(representationArray, 2, "%c", isprint(buf[i+j]) ? buf[i+j] : '.');
				hexCharacterRepresentation += representationArray;
			}

		std::vector<std::string> hexData(2);
		hexData[0] = hex;
		hexData[1] = hexCharacterRepresentation;

		hexDataVector.push_back(hexData);
  	}

	delete[] data->buffer;
	data->hexEntries = hexDataVector;
	data->collectedHexEntries = true;

	if (concurrency) pthread_exit(NULL);
	return 0;
}


void *getBinaryDataBufferFromFile(void *arg) {
    hex_data *data = (hex_data *)arg;
    std::string filepath = data->filepath;

    if (!doesFileExist(filepath)) {
		if (concurrency) pthread_exit(NULL);
		return 0;
    }

    std::ifstream ramDumpFile(filepath.c_str(), std::ios::in | std::ios::binary);

    std::streampos fsize = 0;

    fsize = ramDumpFile.tellg();
    ramDumpFile.seekg(0, std::ios::end);
    fsize = ramDumpFile.tellg() - fsize;
    data->fsize = fsize;

    ramDumpFile.clear();
    ramDumpFile.seekg(0, std::ios::beg);


    data->buffer = new char[fsize];
    ramDumpFile.read(data->buffer, fsize);
    ramDumpFile.close();

	if (concurrency) {
		data->collectedBufferData = true;

		pthread_t getHexEntriesThread;
	    data->rcGetHexEntriesThread = pthread_create(&getHexEntriesThread, NULL, collectHexDataFromBuffer, arg);
		pthread_exit(NULL);
	} else {
		data->collectedBufferData = true;

		collectHexDataFromBuffer(arg);
	}

	return 0;
}

void *compareHexEntriesThread(void *arg) {
    hex_compare *data = (hex_compare *)arg;

    while (!data->before->collectedHexEntries || !data->after->collectedHexEntries){}
    if (data->before->rcGetHexEntriesThread || data->after->rcGetHexEntriesThread) {
        pthread_exit(NULL);
    }

    for (int i = 0; i < data->before->hexEntries.size(); ++i) {
        std::string beforeHex = data->before->hexEntries.at(i).at(0);
        std::string afterHex = data->after->hexEntries.at(i).at(0);

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
                std::string addressStr = address;

                std::vector<std::string> differenceVector(5);
                differenceVector[0] = addressStr;
                differenceVector[1] = beforeFirstByte;
                differenceVector[2] = data->before->hexEntries.at(i).at(1).substr(0, 4);
                differenceVector[3] = afterFirstByte;
                differenceVector[4] = data->after->hexEntries.at(i).at(1).substr(0, 4);

                data->differences.push_back(differenceVector);
            }

            // Second byte
            std::string beforeSecondByte = beforeHex.substr(8, 8);
            std::string afterSecondByte = afterHex.substr(8, 8);
            if (beforeSecondByte != afterSecondByte) {
                char address[8];
                snprintf(address, 8, "%07x", (i*16) + 4);
                std::string addressStr = address;

                std::vector<std::string> differenceVector(5);
                differenceVector[0] = addressStr;
                differenceVector[1] = beforeSecondByte;
                differenceVector[2] = data->before->hexEntries.at(i).at(1).substr(4, 4);
                differenceVector[3] = afterSecondByte;
                differenceVector[4] = data->after->hexEntries.at(i).at(1).substr(4, 4);

                data->differences.push_back(differenceVector);
            }

            // Third byte
            std::string beforeThirdByte = beforeHex.substr(16, 8);
            std::string afterThirdByte = afterHex.substr(16, 8);
            if (beforeThirdByte != afterThirdByte) {
                char address[8];
                snprintf(address, 8, "%07x", (i*16) + 8);
                std::string addressStr = address;

                std::vector<std::string> differenceVector(5);
                differenceVector[0] = addressStr;
                differenceVector[1] = beforeThirdByte;
                differenceVector[2] = data->before->hexEntries.at(i).at(1).substr(8, 4);
                differenceVector[3] = afterThirdByte;
                differenceVector[4] = data->after->hexEntries.at(i).at(1).substr(8, 4);

                data->differences.push_back(differenceVector);
            }

            // Fourth byte
            std::string beforeFourthByte = beforeHex.substr(24, 8);
            std::string afterFourthByte = afterHex.substr(24, 8);
            if (beforeFourthByte != afterFourthByte) {
                char address[8];
                snprintf(address, 8, "%07x", (i*16) + 12);
                std::string addressStr = address;

                std::vector<std::string> differenceVector(5);
                differenceVector[0] = addressStr;
                differenceVector[1] = beforeFourthByte;
                differenceVector[2] = data->before->hexEntries.at(i).at(1).substr(12, 4);
                differenceVector[3] = afterFourthByte;
                differenceVector[4] = data->after->hexEntries.at(i).at(1).substr(12, 4);

                data->differences.push_back(differenceVector);
            }

        }
    }

	if (concurrency) pthread_exit(NULL);

	return 0;
}


int main(int argc, char *argv[]) {
    // Check for help argument
    for (int i=0; i<argc; ++i) {
        std::string argument = argv[i];
        if (argument == "-h" || argument == "--help") {
            std::cout << "\nAvailable arguments:\n\n"
				<< "-s, --sram        Specify filepath to SRAM (MEM1) memory dump file (ram.raw as generated by Dolphin)\n"
                << "-a, --aram        Specify filepath to ARAM (MEM2) memory dump file (aram.raw as generated by Dolphin)\n"
                << "-ns, --no-sram    Disable comparison of SRAM (MEM1) memory dumps\n"
                << "-na, --no-aram    Disable comparison of ARAM (MEM2) memory dumps\n"
                << "-nt, --no-thread  Disable concurrency when processing memory\n"
                << "-h, --help        Print this summary\n\n";

            return 0;
        }
    }

	std::string filepathSram = homeDir + dolphinDumpPath + "ram.raw";
	std::string filepathAram = homeDir + dolphinDumpPath + "aram.raw";
	bool collectFromSram = true;
	bool collectFromAram = true;
	int rc;

	for (int i=1; i<argc; ++i) {
	    std::string argument = argv[i];
	    if (argument == "-s" || argument == "--sram") {
            if (i + 1 == argc) {
                std::cout << "ERROR: SRAM filepath option provided without parameter. Please specify filepath to ram.raw SRAM file in the form of \"--sram /filepath/to/ram.raw\"\n";
                return -1;
            }
            filepathSram = argv[i++];
        }
        else if (argument == "-a" || argument == "--aram") {
            if (i + 1 == argc) {
                std::cout << "ERROR: ARAM filepath option provided without parameter. Please specify filepath to aram.raw ARAM file in the form of \"--aram /filepath/to/aram.raw\"\n";
                return -1;
            }
            filepathAram = argv[i++];
        }
	    else if (argument == "-ns" || argument == "--no-sram") {
	        collectFromSram = false;
	    }
	    else if (argument == "-na" || argument == "--no-aram") {
	        collectFromAram = false;
	    }
	    else if (argument == "-nt" || argument == "--no-thread") {
            concurrency = false;
        }
    }

    std::cout << "\nDolphin Memory Comparator\n\n";

    if (!collectFromSram && !collectFromAram) {
        std::cout << "Both SRAM(MEM1) and ARAM(MEM2) comparisons have been disabled (nothing to do). Program will now exit\n";
        return -1;
    }

    if (collectFromSram && collectFromAram) {
        std::cout << "Collecting both SRAM(MEM1) and ARAM(MEM2)\n\n";
    }
    else if (!collectFromSram) {
        std::cout << "Collecting ARAM(MEM2)\n\n";
    }
    else if (!collectFromAram) {
        std::cout << "Collecting SRAM(MEM1)\n\n";
    }

    std::cout << "Collect first memory dump:\n"
        << "1.) Place breakpoint in Dolphin on desired line\n"
        << "2.) Allow execution to halt on breakpoint\n"
        << "3.) Click "
        << (collectFromSram && collectFromAram ? "both 'Dump MRAM' and 'Dump EXRAM' buttons" : "")
        << (collectFromSram && !collectFromAram ? "'Dump MRAM' button" : "")
        << (!collectFromSram && collectFromAram ? "'Dump EXRAM' button" : "")
        << " in 'Memory' tab to dump memory\n\n";

    std::cout << "Press RETURN key to process first memory dump for comparison...";
    std::cin.get();


    if (collectFromSram) {
        sramHexDataBefore.filepath = filepathSram;
        sramHexDataBefore.collectedBufferData = false;
        sramHexDataBefore.collectedHexEntries = false;

		if (concurrency) {
			pthread_t sramBufferThreadBefore;
			rc = pthread_create(&sramBufferThreadBefore, NULL, getBinaryDataBufferFromFile, &sramHexDataBefore);
			if (rc) {
				std::cout << "ERROR: Thread for collecting SRAM data could not be started. Program will exit\n";
				return -1;
			}
		} else {
			getBinaryDataBufferFromFile(&sramHexDataBefore);
		}
    }

    if (collectFromAram) {
        aramHexDataBefore.filepath = filepathAram;
        aramHexDataBefore.collectedBufferData = false;
        aramHexDataBefore.collectedHexEntries = false;

		if (concurrency) {
			pthread_t aramBufferThreadBefore;
			rc = pthread_create(&aramBufferThreadBefore, NULL, getBinaryDataBufferFromFile, &aramHexDataBefore);
			if (rc) {
				std::cout << "ERROR: Thread for collecting ARAM data could not be started. Program will exit\n";
				return -1;
			}
		} else {
			getBinaryDataBufferFromFile(&aramHexDataBefore);
		}
    }

    while ((collectFromSram && !sramHexDataBefore.collectedBufferData) || (collectFromAram && !aramHexDataBefore.collectedBufferData)) {}
    if (sramHexDataBefore.rcGetHexEntriesThread) {
        std::cout << "ERROR: Thread for collecting SRAM data could not be started. Program will exit\n";
        return -1;
    }
    if (aramHexDataBefore.rcGetHexEntriesThread) {
        std::cout << "ERROR: Thread for collecting ARAM data could not be started. Program will exit\n";
        return -1;
    }



    std::cout << "\nCollect second memory dump:\n"
        << "1.) Advance program execution to desired instruction\n"
        << "2.) Click "
        << (collectFromSram && collectFromAram ? "both 'Dump MRAM' and 'Dump EXRAM' buttons" : "")
        << (collectFromSram && !collectFromAram ? "'Dump MRAM' button" : "")
        << (!collectFromSram && collectFromAram ? "'Dump EXRAM' button" : "")
        << " in 'Memory' tab to dump memory again\n\n";

    std::cout << "Press RETURN key to process second memory dump for comparison...";
    std::cin.get();


	pthread_t sramHexCompareThread;
    pthread_t aramHexCompareThread;

    if (collectFromSram) {
        sramHexDataAfter.filepath = filepathSram;
        sramHexDataAfter.collectedBufferData = false;
        sramHexDataAfter.collectedHexEntries = false;

		sramHexCompare.before = &sramHexDataBefore;
        sramHexCompare.after = &sramHexDataAfter;

		if (concurrency) {
			pthread_t sramBufferThreadAfter;
			rc = pthread_create(&sramBufferThreadAfter, NULL, getBinaryDataBufferFromFile, &sramHexDataAfter);
			if (rc) {
				std::cout << "ERROR: Thread for collecting SRAM data could not be started. Program will exit\n";
				return -1;
			}

			rc = pthread_create(&sramHexCompareThread, NULL, compareHexEntriesThread, &sramHexCompare);
			if (rc) {
				std::cout << "ERROR: Thread for collecting SRAM data could not be started. Program will exit\n";
				return -1;
			}
		} else {
			getBinaryDataBufferFromFile(&sramHexDataAfter);
			compareHexEntriesThread(&sramHexCompare);
		}
    }

    if (collectFromAram) {
        aramHexDataAfter.filepath = filepathAram;
        aramHexDataAfter.collectedBufferData = false;
        aramHexDataAfter.collectedHexEntries = false;

		aramHexCompare.before = &aramHexDataBefore;
        aramHexCompare.after = &aramHexDataAfter;

		if (concurrency) {
			pthread_t aramBufferThreadAfter;
			rc = pthread_create(&aramBufferThreadAfter, NULL, getBinaryDataBufferFromFile, &aramHexDataAfter);
	        if (rc) {
	            std::cout << "ERROR: Thread for collecting ARAM data could not be started. Program will exit\n";
	            return -1;
	        }

	        rc = pthread_create(&aramHexCompareThread, NULL, compareHexEntriesThread, &aramHexCompare);
	        if (rc) {
	            std::cout << "ERROR: Thread for collecting ARAM data could not be started. Program will exit\n";
	            return -1;
	        }
		} else {
			getBinaryDataBufferFromFile(&aramHexDataAfter);
			compareHexEntriesThread(&aramHexCompare);
		}
    }

    std::cout << "\nProcessing data..." << std::flush;

    pthread_join(sramHexCompareThread, NULL);
    pthread_join(aramHexCompareThread, NULL);

	std::cout << "\n";

	if (sramHexDataAfter.rcGetHexEntriesThread) {
        std::cout << "ERROR: Thread for collecting SRAM data could not be started. Program will exit\n";
        return -1;
    }
    if (aramHexDataAfter.rcGetHexEntriesThread) {
        std::cout << "ERROR: Thread for collecting ARAM data could not be started. Program will exit\n";
        return -1;
    }

    std::cout << "Done\n\n";

	if (sramHexCompare.differences.size() + aramHexCompare.differences.size() == 0) {
		std::cout << "No differences found between memory dumps\n\n";
		return 0;
	}

	std::cout << sramHexCompare.differences.size() + aramHexCompare.differences.size() << " differences found in the format:\n"
        << "Address(Number in list)\n--------\nHex value Before |ASCII|\nHex value After  |ASCII|\n\n";


    for (int i=0; i<sramHexCompare.differences.size(); ++i) {
        std::cout << "8" << sramHexCompare.differences.at(i).at(0) << "(" << i+1 << ")\n--------\n"
            << sramHexCompare.differences.at(i).at(1) << " |" << sramHexCompare.differences.at(i).at(2) << "|\n"
            << sramHexCompare.differences.at(i).at(3) << " |" << sramHexCompare.differences.at(i).at(4) << "|\n\n";
    }
    for (int i=0; i<aramHexCompare.differences.size(); ++i) {
        std::cout << 9 << aramHexCompare.differences.at(i).at(0) << "(" << i+1 << ")\n--------\n"
            << aramHexCompare.differences.at(i).at(1) << " |" << aramHexCompare.differences.at(i).at(2) << "|\n"
            << aramHexCompare.differences.at(i).at(3) << " |" << aramHexCompare.differences.at(i).at(4) << "|\n\n";
    }
}
