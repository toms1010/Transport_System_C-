#include "utils/FileUtils.hpp"

#include "utils/TextUtils.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace st {

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream in(path, std::ios::binary);
    if (!in) return lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        lines.push_back(line);
    }
    return lines;
}

std::string readAll(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

bool fileExists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

void writeLines(const std::string& path, const std::vector<std::string>& lines) {
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return;
        for (const std::string& l : lines) out << l << "\n";
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(path, ec);
        ec.clear();
        std::filesystem::rename(tmp, path, ec);
    }
}

void appendLine(const std::string& path, const std::string& line) {
    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out) return;
    out << line << "\n";
}

void ensureDirectory(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
}

void appendLog(const std::string& path, const std::string& message) {
    appendLine(path, util::dateStamp() + "  " + message);
}

}  // namespace st