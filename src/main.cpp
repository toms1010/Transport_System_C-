#include "repositories/ComplaintRepository.hpp"
#include "repositories/PaymentRepository.hpp"
#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "services/ComplaintService.hpp"
#include "services/PaymentService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "services/UserService.hpp"
#include "ui/ConsoleUI.hpp"
#include "ui/Screens.hpp"
#include "utils/FileUtils.hpp"
#include "models/Admin.hpp"
#include "utils/Security.hpp"
#include "utils/TextUtils.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace {

using namespace st;

const char* kDataDir = "data";
const char* kLogFile = "data/migration.log";

void ensureLayout() {
    // std::filesystem works the same on Linux, macOS and Windows.
    std::error_code ec;
    std::filesystem::create_directories(kDataDir, ec);
}

std::string dataPath(const char* name) { return std::string(kDataDir) + "/" + name; }

// Creates the built-in administrator on a fresh install so the system is never
// left without a way in. The default secret is printed once, at first run only.
void seedAdministrator(AuthService& auth, ConfigService& config) {
    for (const auto& user : auth.userCache()) {
        if (dynamic_cast<const Admin*>(user.get()) != nullptr) return;
    }
    if (auth.usernameExists("admin")) return;

    const std::string secret = "admin123";
    Admin admin("ADM0001", "admin", "Transport Administrator");
    admin.setFatherName("-");
    admin.setPhone("0000000000");
    admin.setAddress(config.config().institutionName + ", " + config.config().institutionCity);
    admin.setCreatedAt(util::nowStamp());

    std::string salt;
    std::string digest;
    Security::hashPassword(secret, salt, digest);
    admin.setSalt(salt);
    admin.setPasswordHash(digest);

    auth.userCache().push_back(std::make_unique<Admin>(admin));
    appendLog(kLogFile, "Bootstrapped administrator account 'admin'.");
    ui::banner("FIRST RUN SETUP", config.config().tagline());
    ui::note("An administrator account was created for you.", ui::StatusKind::Ok);
    ui::field("Username", "admin");
    ui::field("Password", secret);
    ui::note("Change this password immediately after signing in.", ui::StatusKind::Warning);
    appendLog(kLogFile, "Default administrator secret was issued on first run.");
    ui::pause();
}

// One-shot import of the pre-refactor files (list_of_students / login.txt).
// A marker file keeps it from running twice, and every run is recorded in
// data/migration.log so the import is auditable.
void runMigrationIfNeeded(AuthService& auth) {
    const char* kMarker = "data/.legacy_migrated";
    if (fileExists(kMarker)) return;
    if (!fileExists("list_of_students") && !fileExists("login.txt")) {
        appendLine(kMarker, util::nowStamp());
        return;
    }

    int students = 0;
    int staffAccounts = 0;
    int skipped = 0;

    for (const std::string& line : readLines("list_of_students")) {
        const std::vector<std::string> parts = util::tokenizeWhitespace(line);
        if (parts.size() < 3) {
            ++skipped;
            continue;
        }
        if (auth.usernameExists(parts[0])) {
            ++skipped;
            continue;
        }

        const bool isStaff = util::startsWith(util::upper(parts[0]), "STAF");
        const std::string shortId =
            parts[0].size() > 4 ? parts[0].substr(parts[0].size() - 4) : parts[0];

        std::string salt;
        std::string digest;
        Security::hashPassword("legacy", salt, digest);

        if (isStaff) {
            Staff s("STF" + util::padLeft(shortId, 4, '0'),
                    parts[0], parts.size() > 1 ? parts[1] : parts[0]);
            s.setFatherName(parts.size() > 2 ? parts[2] : "-");
            s.setPhone(parts.size() > 3 ? parts[3] : "0000000000");
            s.setAddress(parts.size() > 4 ? parts[4] : "-");
            s.setCreatedAt(util::nowStamp());
            s.setSalt(salt);
            s.setPasswordHash(digest);
            auth.userCache().push_back(std::make_unique<Staff>(s));
            ++staffAccounts;
        } else {
            Student s("STU" + util::padLeft(shortId, 4, '0'),
                      parts[0], parts.size() > 1 ? parts[1] : parts[0]);
            s.setFatherName(parts.size() > 2 ? parts[2] : "-");
            s.setPhone(parts.size() > 3 ? parts[3] : "0000000000");
            s.setAddress(parts.size() > 4 ? parts[4] : "-");
            s.setCreatedAt(util::nowStamp());
            s.setRegistrationFee(2500);
            s.setSalt(salt);
            s.setPasswordHash(digest);
            auth.userCache().push_back(std::make_unique<Student>(s));
            ++students;
        }
    }

    appendLog(kLogFile, "Legacy migration completed.");
    appendLog(kLogFile, "  Imported students: " + std::to_string(students));
    appendLog(kLogFile, "  Imported staff accounts: " + std::to_string(staffAccounts));
    appendLog(kLogFile, "  Skipped records: " + std::to_string(skipped));
    appendLog(kLogFile, "  All imported accounts use the placeholder secret 'legacy'.");

    appendLine(kMarker, util::nowStamp());
}
}

int main() {
    std::ios::sync_with_stdio(false);
    st::ui::init();
    ensureLayout();

    CsvUserRepository userRepo(dataPath("users.csv"));
    CsvRouteRepository routeRepo(dataPath("routes.csv"));
    CsvPaymentRepository paymentRepo(dataPath("payments.csv"));
    CsvComplaintRepository complaintRepo(dataPath("complaints.csv"));
    CsvNoticeRepository noticeRepo(dataPath("notices.csv"));

    ConfigService config;
    config.load(dataPath("config.csv"));

    AuthService auth(userRepo, paymentRepo, config);
    auth.loadUsers();
    runMigrationIfNeeded(auth);
    seedAdministrator(auth, config);

    if (!auth.userCache().empty()) userRepo.saveAll(auth.userCache());

    auth.loadJournal();

    RouteService routes(auth, routeRepo);
    routes.load();

    SeatService seats(routes, auth, userRepo);
    PaymentService payments(paymentRepo, userRepo, auth, config);
    payments.load();
    ComplaintService complaints(complaintRepo, auth);
    complaints.load();
    NoticeService notices(noticeRepo);
    notices.load();

    ui::Screens screens(auth, routes, seats, payments, complaints, notices, config);
    screens.run();

    return 0;
}