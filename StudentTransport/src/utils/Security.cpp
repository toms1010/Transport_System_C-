#include "utils/Security.hpp"

#include "utils/TextUtils.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <sstream>

#ifdef _WIN32
#include <process.h>
#define ST_GETPID _getpid
#else
#include <unistd.h>
#define ST_GETPID getpid
#endif

namespace st {
namespace {

const std::size_t kMinSecretLength = 8;
const std::size_t kMaxSecretLength = 128;

std::uint64_t fnv1a(const std::string& data, std::uint64_t seed) {
    std::uint64_t h = seed;
    for (char raw : data) {
        const auto c = static_cast<unsigned char>(raw);
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

std::uint64_t avalanche(std::uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

std::string derive(const std::string& salt, const std::string& secret) {
    std::uint64_t h = fnv1a(secret + "\x1f" + salt, 1469598103934665603ULL);
    for (std::uint64_t round = 0; round < 8192; ++round) {
        h = avalanche(h ^ (round + 0x9e3779b97f4a7c15ULL));
    }
    std::ostringstream out;
    out << std::hex << h;
    return out.str();
}

unsigned long long processEntropy() { return static_cast<unsigned long long>(ST_GETPID()); }

std::string makeSalt() {
    static const char* digits = "0123456789abcdef";
    std::uint64_t state = static_cast<std::uint64_t>(static_cast<unsigned long long>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    state ^= static_cast<std::uint64_t>(processEntropy()) * 2654435761ULL;
    std::string out;
    for (int i = 0; i < 16; ++i) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        out.push_back(digits[static_cast<std::size_t>((state >> 33) & 0xFU)]);
    }
    return out;
}

}

bool Security::hashPassword(const std::string& secret, std::string& salt, std::string& digest) {
    if (secret.empty()) return false;
    salt = makeSalt();
    digest = derive(salt, secret);
    return true;
}

VerifyResult Security::verifyPassword(const std::string& secret, const std::string& salt,
                                      const std::string& expectedDigest) {
    if (secret.empty()) return VerifyResult::EmptySecret;
    if (expectedDigest.empty()) return VerifyResult::MissingHash;
    if (!looksLikeDigest(expectedDigest)) {
        return VerifyResult::MissingHash;
    }
    if (constantTimeEquals(derive(salt, secret), expectedDigest)) return VerifyResult::Ok;
    return VerifyResult::Mismatch;
}

bool Security::constantTimeEquals(const std::string& a, const std::string& b) {
    unsigned char diff = static_cast<unsigned char>(a.size() ^ b.size());
    const std::size_t n = std::max(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const unsigned char x = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
        const unsigned char y = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
        diff = static_cast<unsigned char>(diff | (x ^ y));
    }
    return diff == 0;
}

bool Security::looksLikeDigest(const std::string& value) {
    if (value.size() < 8) return false;
    for (char c : value) {
        if (std::isxdigit(static_cast<unsigned char>(c)) == 0) return false;
    }
    return true;
}

bool Security::checkPolicy(const std::string& secret, std::string& reason) {
    if (secret.size() < kMinSecretLength) {
        reason = "Password must be at least " + std::to_string(kMinSecretLength) + " characters.";
        return false;
    }
    if (secret.size() > kMaxSecretLength) {
        reason = "Password must be at most " + std::to_string(kMaxSecretLength) + " characters.";
        return false;
    }
    bool hasLetter = false;
    bool hasDigit = false;
    for (char c : secret) {
        if (std::isalpha(static_cast<unsigned char>(c))) hasLetter = true;
        if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
    }
    if (!hasLetter || !hasDigit) {
        reason = "Password must contain both letters and digits.";
        return false;
    }
    return true;
}

HashStrength Security::rateStrength(const std::string& secret) {
    int score = 0;
    if (secret.size() >= kMinSecretLength) ++score;
    if (secret.size() >= 12) ++score;
    if (secret.size() >= 16) ++score;
    bool hasLetter = false;
    bool hasDigit = false;
    bool hasSymbol = false;
    for (char c : secret) {
        if (std::isalpha(static_cast<unsigned char>(c))) hasLetter = true;
        else if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
        else hasSymbol = true;
    }
    if (hasLetter && hasDigit) ++score;
    if (hasSymbol) ++score;
    const int clamped = std::max(0, std::min(score, 4));
    return static_cast<HashStrength>(clamped);
}

std::string Security::strengthLabel(HashStrength strength) {
    switch (strength) {
        case HashStrength::Weak: return "weak";
        case HashStrength::Fair: return "fair";
        case HashStrength::Good: return "good";
        case HashStrength::Strong: return "strong";
        case HashStrength::Unusable:
        default: return "unusable";
    }
}

std::string Security::makeReference(const std::string& prefix) {
    std::uint64_t state = static_cast<std::uint64_t>(static_cast<unsigned long long>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    state ^= static_cast<std::uint64_t>(processEntropy()) * 1099511628211ULL;
    std::ostringstream out;
    out << prefix;
    static const char* digits = "0123456789ABCDEF";
    for (int i = 0; i < 10; ++i) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        out << digits[static_cast<std::size_t>((state >> 33) & 0xFU)];
    }
    return out.str();
}

}  // namespace st