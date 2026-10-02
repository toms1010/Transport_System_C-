#include "TestHarness.hpp"

#include "repositories/ComplaintRepository.hpp"
#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "services/ComplaintService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Security.hpp"
#include "utils/TextUtils.hpp"
#include "utils/Validation.hpp"

#include <filesystem>
#include <limits>

using namespace st;

namespace {

struct RobustFixture {
    std::filesystem::path dir;
    CsvUserRepository users;
    CsvRouteRepository routesRepo;
    CsvPaymentRepository paymentsRepo;
    CsvComplaintRepository complaintRepo;
    CsvNoticeRepository noticeRepo;
    ConfigService config;
    AuthService auth;
    RouteService routes;
    SeatService seats;
    ComplaintService complaints;
    NoticeService notices;

    explicit RobustFixture(const std::string& name)
        : dir(std::filesystem::temp_directory_path() / ("st_test_robust_" + name)),
          users((dir / "users.csv").string()),
          routesRepo((dir / "routes.csv").string()),
          paymentsRepo((dir / "payments.csv").string()),
          complaintRepo((dir / "complaints.csv").string()),
          noticeRepo((dir / "notices.csv").string()),
          config(),
          auth(users, paymentsRepo, config),
          routes(auth, routesRepo),
          seats(routes, auth, users),
          complaints(complaintRepo, auth),
          notices(noticeRepo) {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        config.load((dir / "config.csv").string());
        auth.loadUsers();
        auth.loadJournal();
        routes.load();
        complaints.load();
        notices.load();
    }

    ~RobustFixture() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    void seed(const std::string& name, const std::string& body) {
        writeLines((dir / name).string(), {body});
    }

    // Re-reads everything from disk, the way a fresh process start-up would.
    void reload() {
        auth.loadUsers();
        auth.loadJournal();
        routes.load();
        complaints.load();
        notices.load();
    }

    std::string addStudent(const std::string& username, const std::string& secret) {
        RegistrationForm f;
        f.fullName = username;
        f.fatherName = "Parent";
        f.phone = "9876543210";
        f.address = "Campus";
        f.username = username;
        f.password = secret;
        auth.registerStudent(f);
        for (const auto& u : auth.userCache()) {
            if (u->username() == username) return u->userId();
        }
        return "";
    }
};

}

ST_TEST(missingFilesAreHandledNotFatal) {
    RobustFixture fx("missing");
    // Every repository is pointed at a directory that has no files at all.
    EXPECT_TRUE(fx.users.loadAll().empty());
    EXPECT_TRUE(fx.paymentsRepo.loadAll().empty());
    EXPECT_TRUE(fx.complaints.cache().empty());
    EXPECT_TRUE(fx.notices.cache().empty());
    Session session;
    EXPECT_TRUE(fx.auth.login("nobody", "secret123", session) == AuthOutcome::UnknownUser);
}

ST_TEST(garbageRowsAreSkippedNotFatal) {
    RobustFixture fx("garbage");
    fx.seed("users.csv",
            "STU0001|asha|STUDENT|Asha|P|9876543210|C|abc|def|2026-01-01 00:00|1|R1|3|2500|0|9000|0|\n"
            "this-row-has-no-separators");
    fx.seed("payments.csv", "not|enough|fields\nPAY00001|STU0001|100|TRANSPORT|2026|x|y");
    fx.seed("notices.csv", "single");
    fx.seed("complaints.csv",
            "CMP0001|x|y\n"                       // too few fields -> skipped
            "CMP0002|STU0001|Real subject|Body|2026-01-01 00:00|OPEN|||");
    fx.seed("routes.csv", "R1");

    fx.reload();

    // Truncated rows are dropped, well-formed rows still load.
    EXPECT_EQ(fx.complaints.cache().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(fx.paymentsRepo.loadAll().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(fx.notices.cache().size(), static_cast<std::size_t>(0));
    EXPECT_EQ(fx.auth.userCache().size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(fx.auth.findStudent("STU0001") != nullptr);
}

ST_TEST(nonNumericNumericFieldsAreRejectedNotWrapped) {
    RobustFixture fx("nonnumeric");
    // seatNumber / fee fields contain junk; the loader must clamp to safe values
    // rather than propagating atoi overflow.
    fx.seed("users.csv",
            "STU0001|asha|STUDENT|Asha|P|9876543210|C|h|s|2026-01-01 00:00|1|R1|abc|xyz|99999999999999999999|"
            "-5|0|");
    fx.seed("routes.csv", "R1|Name|A|B|via|notanumber|notanumber|1");
    fx.reload();

    const Student* s = fx.auth.findStudent("STU0001");
    EXPECT_TRUE(s != nullptr);
    EXPECT_EQ(s->seatNumber(), 0);
    EXPECT_EQ(s->registrationFee(), 0LL);
    EXPECT_EQ(s->transportPaid(), 0LL);

    const Route* r = fx.routes.find("R1");
    EXPECT_TRUE(r != nullptr);
    EXPECT_EQ(r->fare(), 0LL);
    EXPECT_EQ(r->capacity(), 0);
}

ST_TEST(pipedAndPercentFieldsRoundTrip) {
    RobustFixture fx("escaping");
    const std::string id = fx.addStudent("pipey", "secret123");

    std::string detail;
    EXPECT_TRUE(fx.seats.assign(id, "R1", 4, detail) == SeatResult::Success);

    // Field separators and newlines must survive a save/load cycle intact.
    std::string created;
    EXPECT_TRUE(fx.complaints.submit(id, "Route | delay| 100%", "Line one\nLine two | piped",
                                     created) == ComplaintResult::Success);

    ComplaintService reloaded(fx.complaintRepo, fx.auth);
    reloaded.load();
    const Complaint* c = reloaded.find(created);
    EXPECT_TRUE(c != nullptr);
    if (c) {
        EXPECT_EQ(c->subject(), std::string("Route | delay| 100%"));
        EXPECT_EQ(c->description(), std::string("Line one\nLine two | piped"));
    }
}

ST_TEST(oversizedComplaintIsRejected) {
    RobustFixture fx("oversize");
    const std::string id = fx.addStudent("asha", "secret123");
    std::string created;
    const std::string huge(5000, 'x');
    EXPECT_TRUE(fx.complaints.submit(id, "Too long", huge, created) == ComplaintResult::EmptyContent);
    EXPECT_TRUE(fx.complaints.cache().empty());
}

ST_TEST(oversizedNoticeIsRejected) {
    RobustFixture fx("oversizenotice");
    std::string created;
    EXPECT_TRUE(fx.notices.publish("Title", std::string(9000, 'y'), "STF001", created) ==
                NoticeResult::EmptyContent);
    EXPECT_TRUE(fx.notices.cache().empty());
}

ST_TEST(whitespaceOnlyTextIsRejected) {
    RobustFixture fx("whitespace");
    const std::string id = fx.addStudent("asha", "secret123");
    std::string created;
    EXPECT_TRUE(fx.complaints.submit(id, "   ", "\t\n  ", created) == ComplaintResult::EmptyContent);
    std::string noticeId;
    EXPECT_TRUE(fx.notices.publish("  ", "   ", "STF001", noticeId) == NoticeResult::EmptyContent);
}

ST_TEST(strictNumberParsingRejectsNonsense) {
    long long value = 0;
    EXPECT_TRUE(util::parseWholeNumber("42", value));
    EXPECT_EQ(value, 42LL);
    EXPECT_TRUE(util::parseWholeNumber("  7  ", value));
    EXPECT_EQ(value, 7LL);

    EXPECT_TRUE(util::parseWholeNumber("", value) == false);
    EXPECT_TRUE(util::parseWholeNumber("abc", value) == false);
    EXPECT_TRUE(util::parseWholeNumber("-5", value) == false);
    EXPECT_TRUE(util::parseWholeNumber("4.5", value) == false);
    EXPECT_TRUE(util::parseWholeNumber("99999999999999999999999", value) == false);
    EXPECT_TRUE(util::parseWholeNumber("12abc", value) == false);

    int bounded = 0;
    EXPECT_TRUE(util::parseIntInRange("5", 1, 10, bounded));
    EXPECT_TRUE(util::parseIntInRange("11", 1, 10, bounded) == false);
    EXPECT_TRUE(util::parseIntInRange("0", 1, 10, bounded) == false);
    EXPECT_TRUE(util::parseIntInRange("-1", 1, 10, bounded) == false);
}

ST_TEST(moneyFormattingHandlesEdges) {
    EXPECT_EQ(util::money(0), std::string("0"));
    EXPECT_EQ(util::money(1), std::string("1"));
    EXPECT_EQ(util::money(999), std::string("999"));
    EXPECT_EQ(util::money(1000), std::string("1,000"));
    EXPECT_EQ(util::money(1234567), std::string("1,234,567"));
    EXPECT_EQ(util::money(-1234567), std::string("-1,234,567"));
    EXPECT_EQ(util::money(std::numeric_limits<long long>::max()).find(',') != std::string::npos,
              true);
}

ST_TEST(validationRejectsHostileInput) {
    std::string reason;
    EXPECT_TRUE(validate::username(std::string(100, 'a'), reason) == false);
    EXPECT_TRUE(validate::username("has space", reason) == false);
    EXPECT_TRUE(validate::username("has|pipe", reason) == false);
    EXPECT_TRUE(validate::username("ab", reason) == false);
    EXPECT_TRUE(validate::username("valid.name-1", reason) == true);

    EXPECT_TRUE(validate::fullName("Digit2Name", reason) == false);
    EXPECT_TRUE(validate::fullName("", reason) == false);

    EXPECT_TRUE(validate::phone("123", reason) == false);
    EXPECT_TRUE(validate::phone("+91 98765 43210", reason) == true);

    EXPECT_TRUE(validate::seatNumber(1, 0, reason) == false);
    EXPECT_TRUE(validate::capacity(0, reason) == false);
    EXPECT_TRUE(validate::capacity(100000, reason) == false);
    EXPECT_TRUE(validate::fare(-1, reason) == false);
    EXPECT_TRUE(validate::positiveAmount(0, reason) == false);
}

ST_TEST(seatNumbersStayInRangeAfterLoading) {
    RobustFixture fx("seatrange");
    // A hand-edited file with an absurd seat number must not be trusted.
    fx.seed("users.csv",
            "STU0001|asha|STUDENT|Asha|P|9876543210|C|h|s|2026-01-01 00:00|1|R1|999999|2500|0|9000|0|");
    fx.reload();

    const std::vector<int> seats = fx.routes.occupiedSeats("R1");
    for (int s : seats) EXPECT_TRUE(s >= 1 && s <= 500);
}

ST_TEST(duplicateSeatAcrossReloadedFilesIsStillBlocked) {
    RobustFixture fx("dupefile");
    // Two rows claiming seat 3 on R1, loaded from a hand-edited file.
    fx.seed("users.csv",
            "STU0001|a|STUDENT|A|P|9876543210|C|h|s|2026-01-01 00:00|1|R1|3|2500|0|9000|0|\n"
            "STU0002|b|STUDENT|B|P|9876543210|C|h|s|2026-01-01 00:00|1|R1|3|2500|0|9000|0|");
    fx.reload();

    std::string detail;
    const std::string third = fx.addStudent("third", "secret123");
    EXPECT_TRUE(!third.empty());
    // The loader must not hand seat 3 to a third student while it is claimed.
    EXPECT_TRUE(fx.seats.assign(third, "R1", 3, detail) == SeatResult::SeatTaken);
}

ST_TEST(constantTimeEqualsBehaves) {
    EXPECT_TRUE(Security::constantTimeEquals("abc", "abc"));
    EXPECT_TRUE(Security::constantTimeEquals("", ""));
    EXPECT_TRUE(Security::constantTimeEquals("abc", "abcd") == false);
    EXPECT_TRUE(Security::constantTimeEquals("abc", "abd") == false);
}

int main() {
    std::cout << "test_robustness\n";
    return ::st::testing::Registry::instance().run() == 0 ? 0 : 1;
}