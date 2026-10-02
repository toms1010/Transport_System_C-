#pragma once

#include <string>
#include <vector>

namespace st {
namespace util {

std::string trim(const std::string& s);
std::string upper(std::string s);
std::string lower(std::string s);

std::vector<std::string> split(const std::string& s, char sep);
std::string join(const std::vector<std::string>& parts, char sep);

std::string money(long long v);
std::string pad(const std::string& s, int width);
std::string padLeft(const std::string& s, int width, char fill = '0');

// Strict numeric parsing. std::atoi silently returns 0 for "abc" and overflows
// to a negative number for values beyond int range, which is how bad data used
// to reach the model layer.
bool parseWholeNumber(const std::string& s, long long& out);
bool parseIntInRange(const std::string& s, int lo, int hi, int& out);

std::string nowStamp();
std::string dateStamp();

std::string repeat(char c, int count);
bool startsWith(const std::string& s, const std::string& prefix);
std::vector<std::string> tokenizeWhitespace(const std::string& s);

}  // namespace util
}  // namespace st