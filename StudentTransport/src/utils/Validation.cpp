#include "utils/Validation.hpp"

#include "utils/TextUtils.hpp"

#include <algorithm>
#include <cctype>

namespace st {
namespace validate {
namespace {

const std::size_t kMinUsername = 3;
const std::size_t kMaxUsername = 20;
const std::size_t kMinPhoneDigits = 10;
const std::size_t kMaxPhoneDigits = 15;

}

bool longText(const std::string& value, std::size_t maxLength, const char* fieldName,
              std::string& reason) {
    if (value.size() > maxLength) {
        reason = std::string(fieldName) + " must be " + std::to_string(maxLength) +
                 " characters or fewer (got " + std::to_string(value.size()) + ").";
        return false;
    }
    return true;
}

bool complaintDescription(const std::string& value, std::string& reason) {
    if (util::trim(value).empty()) {
        reason = "Complaint description is required.";
        return false;
    }
    return longText(value, 2000, "Complaint description", reason);
}

bool noticeContent(const std::string& value, std::string& reason) {
    if (util::trim(value).empty()) {
        reason = "Notice content is required.";
        return false;
    }
    return longText(value, 4000, "Notice content", reason);
}

bool username(const std::string& value, std::string& reason) {
    const std::string v = util::trim(value);
    if (v.size() < kMinUsername || v.size() > kMaxUsername) {
        reason = "Username must be " + std::to_string(kMinUsername) + " to " +
                 std::to_string(kMaxUsername) + " characters.";
        return false;
    }
    for (char c : v) {
        const bool allowed = std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' ||
                             c == '-';
        if (!allowed) {
            reason = "Username may only contain letters, digits, dot, underscore or hyphen.";
            return false;
        }
    }
    return true;
}

bool fullName(const std::string& value, std::string& reason) {
    const std::string v = util::trim(value);
    if (v.empty()) {
        reason = "Name is required.";
        return false;
    }
    if (v.size() > 80) {
        reason = "Name is too long (max 80 characters).";
        return false;
    }
    for (char c : v) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            reason = "Name must not contain digits.";
            return false;
        }
    }
    return true;
}

bool phone(const std::string& value, std::string& reason) {
    std::string digits;
    for (char c : value) {
        if (std::isdigit(static_cast<unsigned char>(c))) digits.push_back(c);
    }
    if (digits.size() < kMinPhoneDigits || digits.size() > kMaxPhoneDigits) {
        reason = "Phone number must contain between " + std::to_string(kMinPhoneDigits) +
                 " and " + std::to_string(kMaxPhoneDigits) + " digits.";
        return false;
    }
    return true;
}

bool address(const std::string& value, std::string& reason) {
    const std::string v = util::trim(value);
    if (v.empty()) {
        reason = "Address is required.";
        return false;
    }
    if (v.size() > 160) {
        reason = "Address must be 160 characters or fewer.";
        return false;
    }
    return true;
}

bool routeId(const std::string& value, std::string& reason) {
    const std::string v = util::upper(util::trim(value));
    if (v.empty()) {
        reason = "Route id is required.";
        return false;
    }
    if (v.size() > 8) {
        reason = "Route id is too long.";
        return false;
    }
    for (char c : v) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') {
            reason = "Route id may only contain letters, digits or hyphen.";
            return false;
        }
    }
    return true;
}

bool seatNumber(int value, int capacity, std::string& reason) {
    if (capacity <= 0) {
        reason = "This route has no capacity configured.";
        return false;
    }
    if (value < 1 || value > capacity) {
        reason = "Seat must be between 1 and " + std::to_string(capacity) + ".";
        return false;
    }
    return true;
}

bool positiveAmount(long long value, std::string& reason) {
    if (value <= 0) {
        reason = "Amount must be greater than zero.";
        return false;
    }
    return true;
}

bool complaintSubject(const std::string& value, std::string& reason) {
    const std::string v = util::trim(value);
    if (v.empty()) {
        reason = "Complaint subject is required.";
        return false;
    }
    if (v.size() > 80) {
        reason = "Complaint subject must be 80 characters or fewer.";
        return false;
    }
    return true;
}

bool noticeTitle(const std::string& value, std::string& reason) {
    const std::string v = util::trim(value);
    if (v.empty()) {
        reason = "Notice title is required.";
        return false;
    }
    if (v.size() > 80) {
        reason = "Notice title must be 80 characters or fewer.";
        return false;
    }
    return true;
}

bool fare(long long value, std::string& reason) {
    if (value < 0) {
        reason = "Fare cannot be negative.";
        return false;
    }
    if (value > 100000000LL) {
        reason = "Fare value is unrealistically large.";
        return false;
    }
    return true;
}

bool capacity(int value, std::string& reason) {
    if (value < 1 || value > 500) {
        reason = "Capacity must be between 1 and 500.";
        return false;
    }
    return true;
}

}  // namespace validate
}  // namespace st