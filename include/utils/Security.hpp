#pragma once

#include <string>

namespace st {

enum class VerifyResult {
    Ok,
    EmptySecret,
    MissingHash,
    Mismatch,
};

enum class HashStrength { Unusable, Weak, Fair, Good, Strong };

// Password storage for the console build.
//
// This is a salted, iterated, non-cryptographic-library digest (FNV-1a seeded
// with a per-user 64-bit salt, then mixed through 8192 avalanche rounds).
// It is deliberately isolated behind this header so that swapping in Argon2,
// bcrypt, scrypt or PBKDF2 later touches exactly one file.
//
// It is NOT a substitute for a production password KDF. See docs/architecture.md.
class Security {
public:
    static bool hashPassword(const std::string& secret, std::string& salt, std::string& digest);

    static VerifyResult verifyPassword(const std::string& secret, const std::string& salt,
                                       const std::string& expectedDigest);

    static bool constantTimeEquals(const std::string& a, const std::string& b);

    static bool looksLikeDigest(const std::string& value);

    static bool checkPolicy(const std::string& secret, std::string& reason);

    static HashStrength rateStrength(const std::string& secret);
    static std::string strengthLabel(HashStrength strength);

    static std::string makeReference(const std::string& prefix);
};

}  // namespace st