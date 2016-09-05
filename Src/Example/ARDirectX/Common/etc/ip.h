//
// IP Address
//
#pragma once

#include <vector>
#include <string>

namespace common
{

	string GetHostIP(const int networkCardIndex);

	void GetHostIPAll(std::vector<std::string> &ips);

	uint32_t parseIPV4string(const string &ipAddress);

}

