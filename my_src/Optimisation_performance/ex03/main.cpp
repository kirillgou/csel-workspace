#include "ApacheAccessLogAnalyzer.h"

#include <iostream>

// forward declaration
void usage(const char* progName);

int main(int argc, const char* argv[])
{
	if(argc != 2)
    {
        usage(argv[0]);
        return -1;
    }

    std::string filename = argv[1];

    ApacheAccessLogAnalyzer analyzer(filename);

    analyzer.openFile();
    analyzer.processFile();
    analyzer.closeFile();
}


void usage(const char* progName)
{
    std::cout << "Usage: " << progName << " <filename>" << std::endl;
    std::cout << "\nWhere <filename> is the apache access log file" << std::endl;
}
