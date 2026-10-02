#include "utils/TextUtils.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cerrno>
#include <cstdlib>
#include <climits>

namespace st {
namespace util {

std::string trim(const std::string& s) {
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream in(s);
    while (std::getline(in, cur, sep)) out.push_back(cur);
    if (!s.empty() && s.back() == sep) out.push_back("");
    return out;
}

std::string join(const std::vector<std::string>& parts, char sep) {
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i) out.push_back(sep);
        out += parts[i];
    }
    return out;
}

std::string money(long long v) {
    if (v == 0) return "0";
    std::string digits = std::to_string(v < 0 ? -v : v);
    std::string out;
    int count = 0;
    for (std::size_t i = digits.size(); i-- > 0;) {
        out.push_back(digits[i]);
        if (++count % 3 == 0 && i > 0) out.push_back(',');
    }
    if (v < 0) out.push_back('-');
    std::reverse(out.begin(), out.end());
    return out;
}

std::string pad(const std::string& s, int width) {
    std::string out = s;
    while (static_cast<int>(out.size()) < width) out.push_back(' ');
    return out;
}

std::string padLeft(const std::string& s, int width, char fill) {
    std::string out;
    for (int i = static_cast<int>(s.size()); i < width; ++i) out.push_back(fill);
    return out + s;
}

bool parseWholeNumber(const std::string& s, long long& out) {
    const std::string t = trim(s);
    if (t.empty()) return false;
    for (char c : t) {
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) return false;
    }
    errno = 0;
    char* end = nullptr;
    const long long value = std::strtoll(t.c_str(), &end, 10);
    if (errno == ERANGE || end == nullptr || *end != '\0') return false;
    out = value;
    return true;
}

bool parseIntInRange(const std::string& s, int lo, int hi, int& out) {
    long long value = 0;
    if (!parseWholeNumber(s, value)) return false;
    if (value < lo || value > hi) return false;
    out = static_cast<int>(value);
    return true;
}

std::string nowStamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
    return buf;
}

std::string dateStamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return buf;
}

std::string repeat(char c, int count) {
    std::string out;
    for (int i = 0; i < count; ++i) out.push_back(c);
    return out;
}

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> tokenizeWhitespace(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream in(s);
    std::string t;
    while (in >> t) out.push_back(t);
    return out;
}

}  // namespace util
}  // namespace st