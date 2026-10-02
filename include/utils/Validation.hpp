#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace st {

// Every validation rule lives here so that the UI, the services and the tests
// all agree on what "valid" means. No layer re-implements these checks.
namespace validate {

bool username(const std::string& value, std::string& reason);
bool fullName(const std::string& value, std::string& reason);
bool phone(const std::string& value, std::string& reason);
bool address(const std::string& value, std::string& reason);
bool routeId(const std::string& value, std::string& reason);
bool seatNumber(int value, int capacity, std::string& reason);
bool positiveAmount(long long value, std::string& reason);
bool complaintSubject(const std::string& value, std::string& reason);
bool noticeTitle(const std::string& value, std::string& reason);
bool longText(const std::string& value, std::size_t maxLength, const char* fieldName,
              std::string& reason);
bool complaintDescription(const std::string& value, std::string& reason);
bool noticeContent(const std::string& value, std::string& reason);
bool fare(long long value, std::string& reason);
bool capacity(int value, std::string& reason);

}  // namespace validate
}  // namespace st