#include "TestHarness.hpp"

#include "services/AuthService.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Security.hpp"
#include "utils/Validation.hpp"

#include <filesystem>

using namespace st;

namespace {

struct Sandbox {
    std::filesystem::path dir;

    explicit Sandbox(const std::string& name) {
        dir = std::filesystem::temp_directory_path() / ("st_test_" + name);
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
    }
    ~Sandbox() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
    std::string file(const char* name) const { return (dir / name).string(); }
};

struct Fixture {
    Sandbox box;
    CsvUserRepository users;
    CsvRouteRepository routesRepo;
    CsvPaymentRepository paymentsRepo;
    ConfigService config;

    explicit Fixture(const std::string& name)
        : box(name),
          users(box.file("users.csv")),
          routesRepo(box.file("routes.csv")),
          paymentsRepo(box.file("payments.csv")),
          config() {
        config.load(box.file("config.csv"));
        auth.userCache() = users.loadAll();
    }

    AuthService auth{users, paymentsRepo, config};

    RegistrationForm makeForm(const std::string& username, const std::string& password) {
        RegistrationForm f;
        f.fullName = "Asha Rao";
        f.fatherName = "Kiran Rao";
        f.phone = "9876543210";
        f.address = "Hostel Block C";
        f.username = username;
        f.password = password;
        return f;
    }
};

}

ST_TEST(registerStudentCreatesHashedAccount) {
    Fixture fx("auth_register");
    const auto result = fx.auth.registerStudent(fx.makeForm("asha", "secret123"));
    EXPECT_TRUE(result.ok);

    Session session;
    EXPECT_TRUE(fx.auth.login("asha", "secret123", session) == AuthOutcome::Success);
    EXPECT_EQ(session.role, Role::Student);
    EXPECT_EQ(session.displayName, std::string("Asha Rao"));
}

ST_TEST(loginRejectsWrongPassword) {
    Fixture fx("auth_wrong");
    fx.auth.registerStudent(fx.makeForm("asha", "secret123"));

    Session session;
    EXPECT_TRUE(fx.auth.login("asha", "wrongpass", session) == AuthOutcome::WrongPassword);
    EXPECT_TRUE(session.active() == false);
}

ST_TEST(loginRejectsUnknownUsername) {
    Fixture fx("auth_unknown");
    fx.auth.registerStudent(fx.makeForm("asha", "secret123"));

    Session session;
    EXPECT_TRUE(fx.auth.login("nobody", "secret123", session) == AuthOutcome::UnknownUser);
}

ST_TEST(loginRejectsEmptyInput) {
    Fixture fx("auth_empty");
    Session session;
    EXPECT_TRUE(fx.auth.login("", "", session) == AuthOutcome::EmptyInput);
}

ST_TEST(duplicateUsernameIsRejected) {
    Fixture fx("auth_dupe");
    fx.auth.registerStudent(fx.makeForm("asha", "secret123"));
    const auto again = fx.auth.registerStudent(fx.makeForm("asha", "another123"));
    EXPECT_TRUE(again.ok == false);
    EXPECT_TRUE(again.message.find("already taken") != std::string::npos);
}

ST_TEST(weakPasswordIsRejected) {
    Fixture fx("auth_weak");
    auto form = fx.makeForm("asha", "short");
    const auto result = fx.auth.registerStudent(form);
    EXPECT_TRUE(result.ok == false);

    std::string reason;
    EXPECT_TRUE(Security::checkPolicy("nodigitshere", reason) == false);
    EXPECT_TRUE(Security::checkPolicy("secret123", reason) == true);
}

ST_TEST(passwordIsNeverStoredInPlaintext) {
    Fixture fx("auth_plain");
    fx.auth.registerStudent(fx.makeForm("asha", "secret123"));
    const std::string raw = readAll(fx.box.file("users.csv"));
    EXPECT_TRUE(raw.find("secret123") == std::string::npos);
}

ST_TEST(staffRegistrationRequiresAuthorizationCode) {
    Fixture fx("auth_staff");
    auto form = fx.makeForm("staff1", "secret123");

    const auto noCode = fx.auth.registerStaff(form, "");
    EXPECT_TRUE(noCode.ok == false);
    EXPECT_TRUE(noCode.message.find("authorization code") != std::string::npos);

    const auto wrongCode = fx.auth.registerStaff(form, "GUESSED-CODE");
    EXPECT_TRUE(wrongCode.ok == false);

    const auto rightCode =
        fx.auth.registerStaff(form, fx.config.config().staffAuthorizationCode);
    EXPECT_TRUE(rightCode.ok);

    Session session;
    EXPECT_TRUE(fx.auth.login("staff1", "secret123", session) == AuthOutcome::Success);
    EXPECT_EQ(session.role, Role::Staff);
}

ST_TEST(authorizationCodeIsConfigurable) {
    Fixture fx("auth_code");
    fx.config.mutableConfig().staffAuthorizationCode = "ROTATED-CODE-9";
    fx.config.save();

    ConfigService reloaded;
    reloaded.load(fx.box.file("config.csv"));
    EXPECT_EQ(reloaded.config().staffAuthorizationCode, std::string("ROTATED-CODE-9"));
}

ST_TEST(changePasswordUpdatesCredentials) {
    Fixture fx("auth_change");
    fx.auth.registerStudent(fx.makeForm("asha", "secret123"));
    std::string userId;
    for (const auto& u : fx.auth.userCache()) {
        if (u->username() == "asha") userId = u->userId();
    }
    EXPECT_TRUE(!userId.empty());

    std::string reason;
    EXPECT_TRUE(fx.auth.setPassword(userId, "brandnew123", reason));
    Session session;
    EXPECT_TRUE(fx.auth.login("asha", "brandnew123", session) == AuthOutcome::Success);
    EXPECT_TRUE(fx.auth.login("asha", "secret123", session) == AuthOutcome::WrongPassword);
}

ST_TEST(inactiveAccountCannotSignIn) {
    Fixture fx("auth_inactive");
    fx.auth.registerStudent(fx.makeForm("asha", "secret123"));
    for (auto& user : fx.auth.userCache()) {
        if (user->username() != "asha") continue;
        user->setActive(false);
        fx.users.update(*user, fx.auth.userCache());
        break;
    }
    Session session;
    EXPECT_TRUE(fx.auth.login("asha", "secret123", session) == AuthOutcome::InactiveAccount);
}

int main() {
    std::cout << "test_auth\n";
    return ::st::testing::Registry::instance().run() == 0 ? 0 : 1;
}