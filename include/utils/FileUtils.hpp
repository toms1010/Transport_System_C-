#pragma once

#include <string>
#include <vector>

namespace st {

std::vector<std::string> readLines(const std::string& path);
std::string readAll(const std::string& path);
bool fileExists(const std::string& path);
void writeLines(const std::string& path, const std::vector<std::string>& lines);
void appendLine(const std::string& path, const std::string& line);
void ensureDirectory(const std::string& path);
void appendLog(const std::string& path, const std::string& message);

}  // namespace st