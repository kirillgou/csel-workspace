#include "HostCounter.h"

#include <algorithm> // for std::find

HostCounter::HostCounter()
{
}

bool HostCounter::isNewHost(std::string hostname)
{
    return std::find(myHosts.begin(), myHosts.end(), hostname) == myHosts.end();
}

void HostCounter::notifyHost(std::string hostname)
{
    // add the host in the list if not already in
    if(isNewHost(hostname))
    {
        myHosts.push_back(hostname);
    }
}

int HostCounter::getNbOfHosts()
{
    return myHosts.size();
}
