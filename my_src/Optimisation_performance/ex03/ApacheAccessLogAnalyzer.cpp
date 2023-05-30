#include "ApacheAccessLogAnalyzer.h"

#include <iostream>
#include <sstream>
#include <algorithm>


ApacheAccessLogAnalyzer::ApacheAccessLogAnalyzer(std::string filename)
    : myFilename(filename)
{

}

void ApacheAccessLogAnalyzer::openFile()
{
    myInFile.open(myFilename.c_str());
}

void ApacheAccessLogAnalyzer::closeFile()
{
    myInFile.close();
}

void ApacheAccessLogAnalyzer::processFile()
{
    std::cout << "Processing log file " << myFilename << std::endl;
    for( std::string line; getline( myInFile, line ); )
    {
        // parse the log line to extract the hostname / ip address
        int space_pos = line.find_first_of(" ");
        std::string hostname = line.substr(0, space_pos);

        myHostCounter.notifyHost(hostname);
    }

    std::cout << "Found " << myHostCounter.getNbOfHosts() << " unique Hosts/IPs" << std::endl;
}
