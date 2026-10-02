#pragma once

#include <string>
#include <vector>

namespace st {

std::string csvEscape(const std::string& raw);
std::string csvUnescape(const std::string& raw);
std::vector<std::string> csvSplit(const std::string& line);
std::string csvJoin(const std::vector<std::string>& fields);

}  // namespace st