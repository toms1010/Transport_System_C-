#include "TestHarness.hpp"

#include "repositories/ComplaintRepository.hpp"
#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "services/ComplaintService.hpp"
#include "services/ComplaintService.hpp"
#include "utils/Validation.hpp"

#include <filesystem>

using namespace st;

namespace {

struct ComplaintFixture {
    std::filesystem::path dir;
    CsvUserRepository users;
    CsvRouteRepository routesRepo;
    CsvPaymentRepository paymentsRepo;
    CsvComplaintRepository complaintRepo;
    CsvNoticeRepository noticeRepo;
    ConfigService config;
    AuthService auth;
    ComplaintService complaints;
    NoticeService notices;

    ComplaintFixture()
        : dir(std::filesystem::temp_directory_path() / "st_test_complaints"),
          users((dir / "users.csv").string()),
          routesRepo((dir / "routes.csv").string()),
          paymentsRepo((dir / "payments.csv").string()),
          complaintRepo((dir / "complaints.csv").string()),
          noticeRepo((dir / "notices.csv").string()),
          config(),
          auth(users, paymentsRepo, config),
          complaints(complaintRepo, auth),
          notices(noticeRepo) {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        config.load((dir / "config.csv").string());
        auth.userCache() = users.loadAll();
        complaints.load();
        notices.load();
    }

    ~ComplaintFixture() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    std::string addStudent(const std::string& username) {
        RegistrationForm f;
        f.fullName = username;
        f.fatherName = "Parent";
        f.phone = "9876543210";
        f.address = "Campus";
        f.username = username;
        f.password = "secret123";
        auth.registerStudent(f);
        for (const auto& user : auth.userCache()) {
            if (user->username() == username) return user->userId();
        }
        return "";
    }
};

}

ST_TEST(submitComplaintCreatesOpenRecord) {
    ComplaintFixture fx;
    const std::string id = fx.addStudent("asha");

    std::string created;
    const ComplaintResult result =
        fx.complaints.submit(id, "Bus delay", "The bus arrived 40 minutes late.", created);
    EXPECT_TRUE(result == ComplaintResult::Success);
    EXPECT_EQ(created, std::string("CMP0001"));

    const Complaint* c = fx.complaints.find(created);
    EXPECT_TRUE(c != nullptr);
    EXPECT_EQ(c->status(), ComplaintStatus::Open);
    EXPECT_EQ(c->studentId(), id);
}

ST_TEST(emptyComplaintIsRejected) {
    ComplaintFixture fx;
    const std::string id = fx.addStudent("asha");

    std::string created;
    EXPECT_TRUE(fx.complaints.submit(id, "", "some text", created) == ComplaintResult::EmptyContent);
    EXPECT_TRUE(fx.complaints.submit(id, "subject", "   ", created) == ComplaintResult::EmptyContent);
    EXPECT_TRUE(fx.complaints.cache().empty());
}

ST_TEST(resolveRecordsResponderAndTimestamp) {
    ComplaintFixture fx;
    const std::string id = fx.addStudent("asha");

    std::string created;
    fx.complaints.submit(id, "Seat problem", "My seat number is not printed.", created);

    std::string message;
    EXPECT_TRUE(fx.complaints.resolve(created, "STF001", "Seat reprinted at the office.", message) ==
                ComplaintResult::Success);

    const Complaint* c = fx.complaints.find(created);
    EXPECT_EQ(c->status(), ComplaintStatus::Resolved);
    EXPECT_EQ(c->resolvedBy(), std::string("STF001"));
    EXPECT_EQ(c->response(), std::string("Seat reprinted at the office."));
    EXPECT_TRUE(!c->resolvedAt().empty());
}

ST_TEST(resolvingUnknownComplaintFails) {
    ComplaintFixture fx;
    std::string message;
    EXPECT_TRUE(fx.complaints.resolve("CMP9999", "STF001", "text", message) ==
                ComplaintResult::NotFound);
    EXPECT_TRUE(message.find("not found") != std::string::npos);
}

ST_TEST(reopenClearsResolutionFields) {
    ComplaintFixture fx;
    const std::string id = fx.addStudent("asha");
    std::string created;
    fx.complaints.submit(id, "Bus delay", "Late again.", created);

    std::string message;
    fx.complaints.resolve(created, "STF001", "Noted.", message);
    EXPECT_TRUE(fx.complaints.reopen(created, "STF002", message) == ComplaintResult::Success);

    const Complaint* c = fx.complaints.find(created);
    EXPECT_EQ(c->status(), ComplaintStatus::Open);
    EXPECT_TRUE(c->resolvedBy().empty());
}

ST_TEST(filteringByStatusAndStudentWorks) {
    ComplaintFixture fx;
    const std::string a = fx.addStudent("asha");
    const std::string b = fx.addStudent("bilal");

    std::string first;
    std::string second;
    fx.complaints.submit(a, "Seat problem", "Broken seat.", first);
    fx.complaints.submit(b, "Late bus", "Forty minutes late.", second);

    EXPECT_EQ(fx.complaints.openCount(), 2);
    std::string message;
    fx.complaints.resolve(first, "STF001", "Fixed.", message);

    EXPECT_EQ(fx.complaints.openCount(), 1);
    EXPECT_EQ(fx.complaints.forStudent(a).size(), static_cast<std::size_t>(1));
    EXPECT_EQ(fx.complaints.byStatus(ComplaintStatus::Resolved).size(),
              static_cast<std::size_t>(1));
}

ST_TEST(complaintsPersistAcrossReload) {
    ComplaintFixture fx;
    const std::string id = fx.addStudent("asha");
    std::string created;
    fx.complaints.submit(id, "Route change", "Wrong route assigned.", created);

    ComplaintService reloaded(fx.complaintRepo, fx.auth);
    reloaded.load();
    EXPECT_EQ(reloaded.cache().size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(reloaded.find(created) != nullptr);
}

ST_TEST(complaintIdSequenceIsMonotonic) {
    ComplaintFixture fx;
    const std::string id = fx.addStudent("asha");
    std::string first;
    std::string second;
    fx.complaints.submit(id, "One", "First issue.", first);
    fx.complaints.submit(id, "Two", "Second issue.", second);
    EXPECT_EQ(first, std::string("CMP0001"));
    EXPECT_EQ(second, std::string("CMP0002"));
}

ST_TEST(noticeLifecycle) {
    ComplaintFixture fx;
    std::string created;
    EXPECT_TRUE(fx.notices.publish("Holiday Schedule", "Buses run on a holiday timetable.",
                                   "STF001", created) == NoticeResult::Success);
    EXPECT_EQ(created, std::string("NOT001"));

    std::string message;
    EXPECT_TRUE(fx.notices.edit(created, "Revised Schedule", "Timetable updated.", message) ==
                NoticeResult::Success);
    EXPECT_EQ(fx.notices.find(created)->title(), std::string("Revised Schedule"));

    EXPECT_TRUE(fx.notices.remove(created, message) == NoticeResult::Success);
    EXPECT_TRUE(fx.notices.find(created) == nullptr);
}

ST_TEST(emptyNoticeIsRejected) {
    ComplaintFixture fx;
    std::string created;
    EXPECT_TRUE(fx.notices.publish("", "body", "STF001", created) == NoticeResult::EmptyContent);
    EXPECT_TRUE(fx.notices.cache().empty());
}

ST_TEST(complaintSubjectValidation) {
    std::string reason;
    EXPECT_TRUE(validate::complaintSubject("Bus delay", reason));
    EXPECT_TRUE(validate::complaintSubject("   ", reason) == false);
    EXPECT_TRUE(validate::complaintSubject(std::string(100, 'x'), reason) == false);
}

int main() {
    std::cout << "test_complaints\n";
    return ::st::testing::Registry::instance().run() == 0 ? 0 : 1;
}