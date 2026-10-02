#include "AppContext.hpp"

#include "Widgets.hpp"

#include "models/Admin.hpp"
#include "models/Staff.hpp"
#include "models/Student.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Security.hpp"
#include "utils/TextUtils.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace gui {
namespace {

constexpr const char* kDataDirName = "data";
constexpr const char* kLogFile = "data/migration.log";
constexpr const char* kLegacyMarker = "data/.legacy_migrated";

// The console front end resolves its CSV files against the current working
// directory, which puts them in a different place depending on whether the
// binary was launched from the repository root, from build/, or from an IDE.
// The GUI is launched from three different directories in practice, so the data
// directory is resolved explicitly here.
QString resolveDataDirectory() {
    const QByteArray override = qgetenv("ST_DATA_DIR");
    if (!override.isEmpty()) {
        return QDir::fromNativeSeparators(QString::fromLocal8Bit(override));
    }

    const QDir current = QDir::current();
    if (current.exists(QStringLiteral("data"))) {
        return current.filePath(QStringLiteral("data"));
    }

    if (current.exists(QStringLiteral("../data"))) {
        return QDir::cleanPath(current.absoluteFilePath(QStringLiteral("../data")));
    }

    return current.filePath(QStringLiteral("data"));
}

std::string fileIn(const QString& directory, const char* name) {
    return toStd(QDir(directory).filePath(QString::fromLatin1(name)));
}

}  // namespace

AppContext::AppContext()
    : dataDir_(resolveDataDirectory()),
      userRepo_(fileIn(dataDir_, "users.csv")),
      routeRepo_(fileIn(dataDir_, "routes.csv")),
      paymentRepo_(fileIn(dataDir_, "payments.csv")),
      complaintRepo_(fileIn(dataDir_, "complaints.csv")),
      noticeRepo_(fileIn(dataDir_, "notices.csv")),
      auth_(userRepo_, paymentRepo_, config_),
      routes_(auth_, routeRepo_),
      seats_(routes_, auth_, userRepo_),
      payments_(paymentRepo_, userRepo_, auth_, config_),
      complaints_(complaintRepo_, auth_),
      notices_(noticeRepo_) {
    QDir().mkpath(dataDir_);

    config_.load(fileIn(dataDir_, "config.csv"));

    auth_.loadUsers();
    importLegacyAccounts();
    seedAdministrator();
    if (!auth_.userCache().empty()) userRepo_.saveAll(auth_.userCache());
    auth_.loadJournal();

    routes_.load();
    payments_.load();
    complaints_.load();
    notices_.load();
}

QString AppContext::institutionName() const {
    return qs(config_.config().institutionName);
}

void AppContext::importLegacyAccounts() {
    if (st::fileExists(kLegacyMarker)) return;
    if (!st::fileExists("list_of_students") && !st::fileExists("login.txt")) {
        st::appendLine(kLegacyMarker, st::util::nowStamp());
        return;
    }

    int students = 0;
    int staffAccounts = 0;
    int skipped = 0;

    for (const std::string& line : st::readLines("list_of_students")) {
        const std::vector<std::string> parts = st::util::tokenizeWhitespace(line);
        if (parts.size() < 3) {
            ++skipped;
            continue;
        }
        if (auth_.usernameExists(parts[0])) {
            ++skipped;
            continue;
        }

        const bool isStaff = st::util::startsWith(st::util::upper(parts[0]), "STAF");
        const std::string shortId =
            parts[0].size() > 4 ? parts[0].substr(parts[0].size() - 4) : parts[0];
        const std::string userId =
            (isStaff ? std::string("STF") : std::string("STU")) + st::util::padLeft(shortId, 4, '0');

        std::string salt;
        std::string digest;
        st::Security::hashPassword("legacy", salt, digest);

        if (isStaff) {
            st::Staff account(userId, parts[0], parts.size() > 1 ? parts[1] : parts[0]);
            account.setFatherName(parts.size() > 2 ? parts[2] : "-");
            account.setPhone(parts.size() > 3 ? parts[3] : "0000000000");
            account.setAddress(parts.size() > 4 ? parts[4] : "-");
            account.setCreatedAt(st::util::nowStamp());
            account.setSalt(salt);
            account.setPasswordHash(digest);
            auth_.userCache().push_back(std::make_unique<st::Staff>(account));
            ++staffAccounts;
        } else {
            st::Student student(userId, parts[0], parts.size() > 1 ? parts[1] : parts[0]);
            student.setFatherName(parts.size() > 2 ? parts[2] : "-");
            student.setPhone(parts.size() > 3 ? parts[3] : "0000000000");
            student.setAddress(parts.size() > 4 ? parts[4] : "-");
            student.setCreatedAt(st::util::nowStamp());
            student.setRegistrationFee(config_.config().registrationFee);
            student.setSalt(salt);
            student.setPasswordHash(digest);
            auth_.userCache().push_back(std::make_unique<st::Student>(student));
            ++students;
        }
    }

    st::appendLog(kLogFile, "Legacy migration completed from the Qt desktop front end.");
    st::appendLog(kLogFile, "  Imported students: " + std::to_string(students));
    st::appendLog(kLogFile, "  Imported staff accounts: " + std::to_string(staffAccounts));
    st::appendLog(kLogFile, "  Skipped records: " + std::to_string(skipped));
    st::appendLog(kLogFile, "  All imported accounts use the placeholder secret 'legacy'.");
    st::appendLine(kLegacyMarker, st::util::nowStamp());
}

void AppContext::seedAdministrator() {
    for (const auto& user : auth_.userCache()) {
        if (dynamic_cast<const st::Admin*>(user.get()) != nullptr) return;
    }
    if (auth_.usernameExists("admin")) return;

    const std::string secret = "admin123";
    st::Admin administrator("ADM0001", "admin", "Transport Administrator");
    administrator.setFatherName("-");
    administrator.setPhone("0000000000");
    administrator.setAddress(config_.config().institutionName + ", " +
                             config_.config().institutionCity);
    administrator.setCreatedAt(st::util::nowStamp());

    std::string salt;
    std::string digest;
    st::Security::hashPassword(secret, salt, digest);
    administrator.setSalt(salt);
    administrator.setPasswordHash(digest);

    auth_.userCache().push_back(std::make_unique<st::Admin>(administrator));
    seededAdministrator_ = true;
    st::appendLog(kLogFile, "Bootstrapped administrator account 'admin' from the Qt desktop front end.");
    st::appendLog(kLogFile, "Default administrator secret was issued on first run.");
}

void AppContext::refreshAll() {
    auth_.loadUsers();
    auth_.loadJournal();
    routes_.reload();
    payments_.load();
    complaints_.load();
    notices_.load();
}

QStringList AppContext::dataFiles() const {
    return {QStringLiteral("users.csv"),     QStringLiteral("routes.csv"),
            QStringLiteral("payments.csv"),  QStringLiteral("complaints.csv"),
            QStringLiteral("notices.csv"),   QStringLiteral("config.csv")};
}

ActionResult AppContext::backupTo(const QString& directory) const {
    QDir target(directory);
    if (!target.mkpath(QStringLiteral("."))) {
        return ActionResult::failure(QStringLiteral("Backup failed"),
                                     QStringLiteral("Could not create %1.").arg(directory));
    }

    int copied = 0;
    QStringList missing;
    for (const QString& name : dataFiles()) {
        const QString source = QDir(dataDir_).filePath(name);
        if (!QFile::exists(source)) {
            missing << name;
            continue;
        }
        if (QFile::copy(source, target.filePath(name))) ++copied;
    }

    if (copied == 0) {
        return ActionResult::failure(QStringLiteral("Backup failed"),
                                     QStringLiteral("No data files were found to copy."));
    }

    QFile marker(target.filePath(QStringLiteral("backup.info")));
    if (marker.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&marker);
        out << QStringLiteral("Student Transport backup") << Qt::endl;
        out << qs(st::util::nowStamp()) << Qt::endl;
        out << institutionName() << Qt::endl;
    }

    if (!missing.isEmpty()) {
        return ActionResult::success(
            QStringLiteral("Backed up %1 file(s) to %2. Not present: %3")
                .arg(copied)
                .arg(directory, missing.join(QStringLiteral(", "))));
    }
    return ActionResult::success(
        QStringLiteral("Backed up %1 file(s) to %2.").arg(copied).arg(directory));
}

ActionResult AppContext::restoreFrom(const QString& directory) {
    QDir source(directory);
    if (!source.exists()) {
        return ActionResult::failure(QStringLiteral("Restore failed"),
                                     QStringLiteral("%1 is not a folder.").arg(directory));
    }

    QStringList absent;
    for (const QString& name : dataFiles()) {
        if (!source.exists(name)) absent << name;
    }
    if (absent.size() == dataFiles().size()) {
        return ActionResult::failure(QStringLiteral("Restore failed"),
                                     QStringLiteral("That folder holds no transport data files."));
    }

    // Refuse a partial restore: a half-replaced dataset mixes two generations of
    // records and is harder to recover from than no restore at all.
    if (!absent.isEmpty()) {
        return ActionResult::failure(
            QStringLiteral("Restore failed"),
            QStringLiteral("This backup is missing %1. Restore needs a complete set.")
                .arg(absent.join(QStringLiteral(", "))));
    }

    for (const QString& name : dataFiles()) {
        if (!QFile::copy(source.filePath(name), QDir(dataDir_).filePath(name))) {
            return ActionResult::failure(QStringLiteral("Restore failed"),
                                         QStringLiteral("Could not write %1.").arg(name));
        }
    }

    refreshAll();
    return ActionResult::success(
        QStringLiteral("Restored the dataset from %1. Sign in again.").arg(directory));
}

std::vector<std::string> AppContext::studentIds() const {
    std::vector<std::string> ids;
    for (const auto& user : auth_.userCache()) {
        if (user->role() == st::Role::Student) ids.push_back(user->userId());
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

QString AppContext::studentName(const std::string& userId) const {
    const st::User* user = auth_.findUser(userId);
    return user ? qs(user->displayName()) : QStringLiteral("Unknown");
}

QString AppContext::routeName(const std::string& routeId) const {
    if (routeId.empty()) return QStringLiteral("Unassigned");
    const st::Route* route = routes_.find(routeId);
    return route ? qs(route->routeName()) : QStringLiteral("Unknown route");
}

}  // namespace gui