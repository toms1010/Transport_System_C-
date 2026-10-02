#pragma once

#include <memory>
#include <string>
#include <vector>

namespace st {

enum class Role { Student, Staff, Admin };

const char* roleName(Role role);
Role roleFromName(const std::string& name);

class User {
public:
    User() = default;
    User(std::string userId, std::string username, Role role, std::string fullName)
        : userId_(std::move(userId)),
          username_(std::move(username)),
          role_(role),
          fullName_(std::move(fullName)) {}

    virtual ~User() = default;

    // Users are always stored as unique_ptr<User> so that the concrete type is
    // preserved. Storing them by value in a vector<User> would slice Staff and
    // Student back to User and make every downcast undefined.
    virtual std::unique_ptr<User> clone() const = 0;

    const std::string& userId() const { return userId_; }
    const std::string& username() const { return username_; }
    Role role() const { return role_; }
    const std::string& fullName() const { return fullName_; }
    const std::string& fatherName() const { return fatherName_; }
    const std::string& phone() const { return phone_; }
    const std::string& address() const { return address_; }
    const std::string& passwordHash() const { return passwordHash_; }
    const std::string& salt() const { return salt_; }
    const std::string& createdAt() const { return createdAt_; }
    bool active() const { return active_; }

    void setUserId(std::string v) { userId_ = std::move(v); }
    void setUsername(std::string v) { username_ = std::move(v); }
    void setFullName(std::string v) { fullName_ = std::move(v); }
    void setFatherName(std::string v) { fatherName_ = std::move(v); }
    void setPhone(std::string v) { phone_ = std::move(v); }
    void setAddress(std::string v) { address_ = std::move(v); }
    void setPasswordHash(std::string v) { passwordHash_ = std::move(v); }
    void setSalt(std::string v) { salt_ = std::move(v); }
    void setCreatedAt(std::string v) { createdAt_ = std::move(v); }
    void setActive(bool v) { active_ = v; }
    void setRole(Role v) { role_ = v; }

    std::string displayName() const { return fullName_.empty() ? username_ : fullName_; }

    virtual std::string roleLabel() const { return roleName(role_); }

protected:
    void copyCommonInto(User& target) const {
        target.userId_ = userId_;
        target.username_ = username_;
        target.role_ = role_;
        target.fullName_ = fullName_;
        target.fatherName_ = fatherName_;
        target.phone_ = phone_;
        target.address_ = address_;
        target.passwordHash_ = passwordHash_;
        target.salt_ = salt_;
        target.createdAt_ = createdAt_;
        target.active_ = active_;
    }

protected:
    std::string userId_;
    std::string username_;
    Role role_ = Role::Student;
    std::string fullName_;
    std::string fatherName_;
    std::string phone_;
    std::string address_;
    std::string passwordHash_;
    std::string salt_;
    std::string createdAt_;
    bool active_ = true;
};

}  // namespace st