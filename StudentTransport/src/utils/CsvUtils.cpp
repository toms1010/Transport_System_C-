#include "utils/CsvUtils.hpp"

#include "utils/TextUtils.hpp"

namespace st {

std::string csvEscape(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (char c : raw) {
        switch (c) {
            case '%': out += "%25"; break;
            case '|': out += "%7C"; break;
            case '\n': out += "%0A"; break;
            case '\r': out += "%0D"; break;
            default: out.push_back(c);
        }
    }
    return out;
}

std::string csvUnescape(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size();) {
        if (raw[i] == '%' && i + 2 < raw.size()) {
            const std::string code = raw.substr(i, 3);
            if (code == "%25") { out.push_back('%'); i += 3; continue; }
            if (code == "%7C") { out.push_back('|'); i += 3; continue; }
            if (code == "%0A") { out.push_back('\n'); i += 3; continue; }
            if (code == "%0D") { out.push_back('\r'); i += 3; continue; }
        }
        out.push_back(raw[i]);
        ++i;
    }
    return out;
}

std::vector<std::string> csvSplit(const std::string& line) {
    std::vector<std::string> out;
    for (const std::string& piece : util::split(line, '|')) out.push_back(csvUnescape(piece));
    return out;
}

std::string csvJoin(const std::vector<std::string>& fields) {
    std::vector<std::string> escaped;
    escaped.reserve(fields.size());
    for (const std::string& f : fields) escaped.push_back(csvEscape(f));
    return util::join(escaped, '|');
}

}  // namespace st