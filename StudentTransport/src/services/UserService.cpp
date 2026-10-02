#include "services/UserService.hpp"

#include "utils/CsvUtils.hpp"
#include "utils/FileUtils.hpp"
#include "utils/TextUtils.hpp"

namespace st {
namespace {

void applyValue(SystemConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "registration_fee") {
        long long fee = 0;
        if (util::parseWholeNumber(value, fee) && fee >= 0) cfg.registrationFee = fee;
    }
    else if (key == "institution_name") cfg.institutionName = value;
    else if (key == "institution_city") cfg.institutionCity = value;
    else if (key == "department_name") cfg.departmentName = value;
    else if (key == "contact_name") cfg.contactName = value;
    else if (key == "contact_designation") cfg.contactDesignation = value;
    else if (key == "contact_phone") cfg.contactPhone = value;
    else if (key == "contact_room") cfg.contactRoom = value;
    else if (key == "contact_hours") cfg.contactHours = value;
    else if (key == "staff_authorization_code") cfg.staffAuthorizationCode = value;
}

std::vector<std::string> buildRows(const SystemConfig& cfg) {
    return {"registration_fee|" + std::to_string(cfg.registrationFee),
            "institution_name|" + csvEscape(cfg.institutionName),
            "institution_city|" + csvEscape(cfg.institutionCity),
            "department_name|" + csvEscape(cfg.departmentName),
            "contact_name|" + csvEscape(cfg.contactName),
            "contact_designation|" + csvEscape(cfg.contactDesignation),
            "contact_phone|" + csvEscape(cfg.contactPhone),
            "contact_room|" + csvEscape(cfg.contactRoom),
            "contact_hours|" + csvEscape(cfg.contactHours),
            "staff_authorization_code|" + csvEscape(cfg.staffAuthorizationCode)};
}

}

std::string SystemConfig::tagline() const {
    return institutionName + "  \xE2\x80\xA2  " + departmentName + ", " + institutionCity;
}

void ConfigService::load(const std::string& path) {
    path_ = path;
    config_ = SystemConfig{};
    const std::vector<std::string> lines = readLines(path);
    if (lines.empty()) {
        save();
        return;
    }
    for (const std::string& line : lines) {
        const std::vector<std::string> f = csvSplit(line);
        if (f.size() < 2) continue;
        applyValue(config_, util::lower(util::trim(f[0])), f[1]);
    }
}

void ConfigService::save() const {
    writeLines(path_, buildRows(config_));
}

}  // namespace st