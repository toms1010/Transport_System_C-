#pragma once

#include <string>
#include <vector>

namespace st {

// Runtime configuration, loaded from data/config.csv so that fees and contact
// details are not baked into the binary.
struct SystemConfig {
    long long registrationFee = 2500;
    std::string institutionName = "CBIT";
    std::string institutionCity = "Hyderabad";
    std::string departmentName = "Transport Cell";
    std::string contactName = "Transport In-charge";
    std::string contactDesignation = "Transport Office";
    std::string contactPhone = "+91 00000 00000";
    std::string contactRoom = "Transport Office";
    std::string contactHours = "09:00 - 16:00, Monday to Saturday";

    // Shared secret a person must present to create a staff account. It lives in
    // data/config.csv so it can be rotated without recompiling. It is NOT a
    // substitute for real identity management - see docs/system-design.md.
    std::string staffAuthorizationCode = "TRANSPORT-OFFICE";

    std::string tagline() const;
};

class ConfigService {
public:
    void load(const std::string& path);
    void save() const;

    const SystemConfig& config() const { return config_; }
    SystemConfig& mutableConfig() { return config_; }

    const std::string& path() const { return path_; }

private:
    SystemConfig config_;
    std::string path_;
};

}  // namespace st