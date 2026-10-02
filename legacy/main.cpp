/*
 * Student Transport Management System
 * Cross-platform console application (Windows / Linux / macOS).
 *
 * Build:  g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o transport_system
 * Run:    ./transport_system        (data files are created automatically)
 *
 * Keys:   Up/Down or 1-9 to choose, Enter to select, Esc to go back,
 *         Y/N on confirmations.
 */

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <cerrno>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {

const int kWidth = 78;
const long long kRegistrationFee = 2500;
const std::size_t kPasswordMin = 4;
const std::size_t kPasswordMax = 32;

enum class Role { Student, Staff };

const char* roleName(Role r) { return r == Role::Student ? "STUDENT" : "STAFF"; }

struct Route {
    std::string id;
    std::string name;
    std::string via;
    long long fare = 0;
    int capacity = 0;
};

struct User {
    std::string username;
    Role role = Role::Student;
    std::string name;
    std::string father;
    std::string phone;
    std::string address;
    std::string routeId;
    int seat = 0;
    long long due = 0;
    bool paid = false;
    std::string salt;
    std::string hash;

    long long totalPayable() const { return due + kRegistrationFee; }
};

struct Notice {
    std::string stamp;
    std::string author;
    std::string text;
};

enum class ComplaintStatus { Open, Resolved };

const char* statusName(ComplaintStatus s) {
    return s == ComplaintStatus::Open ? "OPEN" : "RESOLVED";
}

struct Complaint {
    int id = 0;
    std::string stamp;
    std::string username;
    std::string text;
    ComplaintStatus status = ComplaintStatus::Open;
};

const std::vector<Route>& routes() {
    static const std::vector<Route> table = {
        {"R1", "Ameerpet -> Uppal", "via Kachiguda, Secunderabad", 9000, 30},
        {"R2", "Gachibowli -> Secunderabad", "via Nanakramguda, Banjara Hills", 8500, 30},
        {"R3", "Miyapur -> L.B.Nagar", "via Kukatpally, Moosarambagh", 9500, 25},
        {"R4", "Kukatpally -> Charminar", "via Erragadda, Ameerpet", 8000, 25},
        {"R5", "Begumpet -> Shamshabad", "via Erragadda, Gachibowli", 7000, 20},
    };
    return table;
}

const Route* findRoute(const std::string& id) {
    for (const Route& r : routes()) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

std::string routeLabel(const std::string& id) {
    const Route* r = findRoute(id);
    if (!r) return "(unassigned)";
    return r->id + "  " + r->name;
}

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(const std::string& s) {
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream in(s);
    while (std::getline(in, cur, sep)) out.push_back(cur);
    if (!s.empty() && s.back() == sep) out.push_back("");
    return out;
}

std::string money(long long v) {
    if (v == 0) return "0";
    std::string digits = std::to_string(v < 0 ? -v : v);
    std::string out;
    int count = 0;
    for (std::size_t i = digits.size(); i-- > 0;) {
        out.push_back(digits[i]);
        if (++count % 3 == 0 && i > 0) out.push_back(',');
    }
    if (v < 0) out.push_back('-');
    std::reverse(out.begin(), out.end());
    return out;
}

std::string nowStamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
    return buf;
}

std::string encode(std::string s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '|': out += "%7C"; break;
            case '%': out += "%25"; break;
            case '\n': out += "%0A"; break;
            case '\r': out += "%0D"; break;
            default: out.push_back(c);
        }
    }
    return out;
}

std::string decode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size();) {
        if (s[i] == '%' && i + 2 < s.size()) {
            std::string code = s.substr(i, 3);
            if (code == "%7C") { out.push_back('|'); i += 3; continue; }
            if (code == "%0A") { out.push_back('\n'); i += 3; continue; }
            if (code == "%0D") { out.push_back('\r'); i += 3; continue; }
            if (code == "%25") { out.push_back('%'); i += 3; continue; }
        }
        out.push_back(s[i]);
        ++i;
    }
    return out;
}

std::string randomHex(std::size_t bytes) {
    static const char* digits = "0123456789abcdef";
    std::uint64_t state = static_cast<std::uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    for (unsigned char c : lower(std::to_string(state))) state = state * 6364136223846793005ULL + c;
    std::string out;
    for (std::size_t i = 0; i < bytes; ++i) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        out.push_back(digits[(state >> (i * 5)) & 0xF]);
    }
    return out;
}

std::string hashSecret(const std::string& salt, const std::string& secret) {
    std::uint64_t h = 1469598103934665603ULL;
    std::string material = salt + "\x1f" + secret + "\x1f" + salt;
    for (unsigned char c : material) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    for (int round = 0; round < 4096; ++round) {
        std::uint64_t x = h ^ (static_cast<std::uint64_t>(round) << 32);
        x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33;
        h = h * 31 + x;
    }
    std::ostringstream out;
    out << std::hex << h;
    return out.str();
}

bool verifySecret(const User& u, const std::string& secret) {
    return u.salt.empty() ? u.hash == secret : hashSecret(u.salt, secret) == u.hash;
}

std::string makeSaltAndHash(const std::string& secret, std::string& salt, std::string& hash) {
    salt = randomHex(8);
    hash = hashSecret(salt, secret);
    return salt;
}

}

namespace term {

#ifdef _WIN32
HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
COORD coord;

void init() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

int rawGet() {
    int c = _getch();
    if (c == 0 || c == 224) {
        int ext = _getch();
        switch (ext) {
            case 72: return 1000;
            case 80: return 1001;
            case 75: return 1002;
            case 77: return 1003;
            default: return -1;
        }
    }
    if (c == 3) return 4;
    return c;
}

bool more() { return _kbhit() != 0; }

void gotoXY(int x, int y) {
    coord.X = static_cast<SHORT>(x - 1);
    coord.Y = static_cast<SHORT>(y - 1);
    SetConsoleCursorPosition(console, coord);
}

int timedGet() { return more() ? rawGet() : -1; }

void restore() {}
#else
termios g_saved{};
bool g_raw = false;
termios g_pending{};
bool g_pendingSet = false;

void init() {}

void setRaw() {
    if (g_raw) return;
    if (tcgetattr(STDIN_FILENO, &g_saved) != 0) return;
    termios raw = g_saved;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) g_raw = true;
}

void restore() {
    if (!g_raw) return;
    tcsetattr(STDIN_FILENO, TCSANOW, &g_saved);
    g_raw = false;
}

int rawGet() {
    unsigned char c = 0;
    ssize_t n = ::read(STDIN_FILENO, &c, 1);
    if (n != 1) return -1;
    if (c == 3) return 4;
    return c;
}

int timedGet() {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeval tv{0, 40000};
    int ready = select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv);
    if (ready <= 0) return -1;
    return rawGet();
}

void gotoXY(int x, int y) { std::cout << "\033[" << y << ";" << x << "H"; }

bool more() {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeval tv{0, 0};
    return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv) > 0;
}
#endif

enum Key { Up = 1000, Down, Left, Right, Esc, CtrlD, Unknown };

void flushInput() {
    while (more()) {
        if (rawGet() < 0) break;
    }
}

Key readKey() {
    int c = rawGet();
    if (c < 0) return CtrlD;
    if (c == 27) {
        int first = timedGet();
        if (first == '[' || first == 'O') {
            int code = timedGet();
            switch (code) {
                case 'A': return Up;
                case 'B': return Down;
                case 'C': return Right;
                case 'D': return Left;
                default: return Unknown;
            }
        }
        return Esc;
    }
    if (c == 13 || c == 10) {
        if (c == 10) return Down;
        return static_cast<Key>(10);
    }
    if (c == 127 || c == 8) return Left;
    if (c == 9) return Down;
    return static_cast<Key>(c);
}

int waitForKey() {
    setRaw();
    int c = rawGet();
    restore();
    if (c == 27) {
        timedGet();
        timedGet();
    }
    return c;
}

}

namespace ui {

enum Color { Reset, Bold, Dim, Red, Green, Yellow, Blue, Cyan, Magenta, White };

const std::string& set(Color c) {
    static const std::string codes[] = {"\033[0m",   "\033[1m",   "\033[2m",  "\033[31m",
                                        "\033[32m", "\033[33m", "\033[34m", "\033[36m",
                                        "\033[35m", "\033[97m"};
    return codes[static_cast<int>(c)];
}

std::string pad(const std::string& s, int width) {
    std::string out = s;
    while (static_cast<int>(out.size()) < width) out.push_back(' ');
    return out;
}

std::string paint(Color c, const std::string& text) {
    set(c);
    std::string out = text;
    set(Reset);
    return out;
}

void clear() {
    std::cout << "\033[2J\033[H";
}

void rule(Color c = Dim) {
    set(c);
    for (int i = 0; i < kWidth; ++i) std::cout << "\u2500";
    set(Reset);
    std::cout << "\n";
}

void center(const std::string& text, Color c = White) {
    int pad = (kWidth - static_cast<int>(text.size())) / 2;
    if (pad < 0) pad = 0;
    std::string line(static_cast<std::size_t>(pad), ' ');
    set(c);
    std::cout << line << text;
    set(Reset);
    for (int i = 0; i < kWidth - pad - static_cast<int>(text.size()); ++i) std::cout << ' ';
    std::cout << "\n";
}

void banner() {
    clear();
    center("");
    set(Cyan);
    std::string top = "\u250c";
    for (int i = 0; i < kWidth; ++i) top += "\u2500";
    top += "\u2510";
    std::cout << top << set(Reset) << "\n";
    center("STUDENT TRANSPORT MANAGEMENT SYSTEM", Bold);
    center("CBIT  \u00b7  Transport Cell", Dim);
    set(Cyan);
    std::string bottom = "\u2514";
    for (int i = 0; i < kWidth; ++i) bottom += "\u2500";
    bottom += "\u2518";
    std::cout << bottom << set(Reset) << "\n\n";
}

void header(const std::string& title) {
    std::cout << "\n  " << set(Bold) << title << set(Reset) << "\n";
}

void kv(const std::string& label, const std::string& value, int labelWidth = 24) {
    std::string left = "  " + label;
    while (static_cast<int>(left.size()) <= labelWidth) left.push_back(' ');
    std::cout << left << value << "\n";
}

void note(const std::string& text, Color c = Yellow) {
    std::cout << "  " << set(c) << text << set(Reset) << "\n";
}

void pause(const std::string& prompt = "Press any key to continue") {
    std::cout << "\n  " << set(Dim) << prompt << set(Reset) << " ";
    std::cout.flush();
    term::waitForKey();
    std::cout << "\n";
}

void footer() {
    std::cout << "\n";
    center("Up/Down + Enter to select   \u00b7   1-9 jump to item   \u00b7   Esc goes back", Dim);
    center("itsourcecode.com", Dim);
}

}

namespace app {

struct Store {
    std::vector<User> users;
    std::vector<Notice> notices;
    std::vector<Complaint> complaints;
};

Store db;

Store& data() { return db; }

User* findUser(const std::string& username) {
    for (User& u : db.users) {
        if (u.username == username) return &u;
    }
    return nullptr;
}

const User* findUserConst(const std::string& username) {
    return findUser(username);
}

std::vector<const User*> usersOnRoute(const std::string& routeId) {
    std::vector<const User*> out;
    for (const User& u : db.users) {
        if (u.role == Role::Student && u.routeId == routeId) out.push_back(&u);
    }
    return out;
}

int nextComplaintId() {
    int maxId = 0;
    for (const Complaint& c : db.complaints) maxId = std::max(maxId, c.id);
    return maxId + 1;
}

std::string nextUsername(Role role) {
    std::string prefix = role == Role::Student ? "STU" : "STF";
    int n = 1;
    for (;;) {
        std::ostringstream candidate;
        candidate << prefix << std::setw(3) << std::setfill('0') << n;
        if (!findUser(candidate.str())) return candidate.str();
        ++n;
    }
}

bool usernameTaken(const std::string& name) {
    return findUser(name) != nullptr;
}

std::string readWholeFile(const std::string& path, bool* ok = nullptr) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (ok) *ok = false;
        return {};
    }
    std::ostringstream buf;
    buf << in.rdbuf();
    if (ok) *ok = true;
    return buf.str();
}

std::vector<std::string> readLines(const std::string& path) {
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

void writeLines(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    for (const std::string& l : lines) out << l << "\n";
}

void saveUsers() {
    std::vector<std::string> lines;
    for (const User& u : db.users) {
        std::ostringstream row;
        row << u.username << '|' << (u.role == Role::Student ? 'S' : 'T') << '|' << u.name << '|'
            << u.father << '|' << u.phone << '|' << u.address << '|' << u.routeId << '|' << u.seat
            << '|' << u.due << '|' << (u.paid ? 1 : 0) << '|' << u.salt << '|' << u.hash;
        lines.push_back(encode(row.str()));
    }
    writeLines("data/users.csv", lines);
}

void saveNotices() {
    std::vector<std::string> lines;
    for (const Notice& n : db.notices) {
        lines.push_back(encode(n.stamp + "|" + n.author + "|" + n.text));
    }
    writeLines("data/notices.csv", lines);
}

void saveComplaints() {
    std::vector<std::string> lines;
    for (const Complaint& c : db.complaints) {
        lines.push_back(encode(std::to_string(c.id) + "|" + c.stamp + "|" + c.username + "|" +
                               statusName(c.status) + "|" + c.text));
    }
    writeLines("data/complaints.csv", lines);
}

void saveAll() {
    saveUsers();
    saveNotices();
    saveComplaints();
}

void loadUsers() {
    for (const std::string& raw : readLines("data/users.csv")) {
        std::vector<std::string> f = split(decode(raw), '|');
        if (f.size() < 12) continue;
        User u;
        u.username = f[0];
        u.role = f[1] == "T" ? Role::Staff : Role::Student;
        u.name = f[2];
        u.father = f[3];
        u.phone = f[4];
        u.address = f[5];
        u.routeId = f[6];
        u.seat = std::atoi(f[7].c_str());
        u.due = std::atoll(f[8].c_str());
        u.paid = f[9] == "1";
        u.salt = f[10];
        u.hash = f[11];
        db.users.push_back(u);
    }
}

void loadNotices() {
    for (const std::string& raw : readLines("data/notices.csv")) {
        std::vector<std::string> f = split(decode(raw), '|');
        if (f.size() < 3) continue;
        Notice n;
        n.stamp = f[0];
        n.author = f[1];
        n.text = f[2];
        db.notices.push_back(n);
    }
}

void loadComplaints() {
    for (const std::string& raw : readLines("data/complaints.csv")) {
        std::vector<std::string> f = split(decode(raw), '|');
        if (f.size() < 5) continue;
        Complaint c;
        c.id = std::atoi(f[0].c_str());
        c.stamp = f[1];
        c.username = f[2];
        c.status = upper(f[3]) == "RESOLVED" ? ComplaintStatus::Resolved : ComplaintStatus::Open;
        c.text = f[4];
        db.complaints.push_back(c);
    }
}

std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream in(s);
    std::string t;
    while (in >> t) out.push_back(t);
    return out;
}

void migrateLegacy() {
    bool seeded = false;

    std::vector<std::string> roster = readLines("list_of_students");
    std::vector<std::string> creds = readLines("login.txt");

    for (const std::string& line : roster) {
        std::vector<std::string> t = tokenize(line);
        if (t.size() < 6) continue;
        User u;
        u.username = t[0];
        if (usernameTaken(u.username)) continue;
        u.role = lower(u.username).rfind("staf", 0) == 0 ? Role::Staff : Role::Student;
        u.name = t.size() > 1 ? t[1] : "";
        u.father = t.size() > 2 ? t[2] : "";
        u.phone = t.size() > 3 ? t[3] : "";
        u.address = t.size() > 4 ? t[4] : "";
        int seat = t.size() > 5 ? std::atoi(t[5].c_str()) : 0;
        long long amount = t.size() > 6 ? std::atoll(t[6].c_str()) : 0;

        if (u.role == Role::Student) {
            u.routeId = routes().front().id;
            u.seat = seat > 0 ? ((seat - 1) % routes().front().capacity) + 1 : 0;
        }
        u.due = amount;
        u.paid = amount == 0;

        std::string plain;
        for (const std::string& c : creds) {
            if (c.size() > u.username.size() && c.rfind(u.username, 0) == 0) {
                plain = c.substr(u.username.size());
                break;
            }
        }
        if (plain.empty()) plain = "legacy" + u.username.substr(u.username.size() - 3);
        makeSaltAndHash(plain, u.salt, u.hash);

        db.users.push_back(u);
        seeded = true;
    }

    for (const std::string& line : readLines("complaintbox.cpp")) {
        std::string body = trim(line);
        if (body.empty()) continue;
        Complaint c;
        c.id = nextComplaintId();
        c.stamp = nowStamp();
        c.username = "imported";
        c.text = body;
        db.complaints.push_back(c);
        seeded = true;
    }

    if (!readLines("program.txt").empty()) {
        std::string body = trim(readWholeFile("program.txt"));
        if (!body.empty()) {
            Notice n;
            n.stamp = nowStamp();
            n.author = "Transport Cell (imported)";
            n.text = body;
            db.notices.push_back(n);
            seeded = true;
        }
    }

    if (seeded) {
        saveAll();
        ui::note("Imported legacy data from list_of_students / login.txt.", ui::Green);
    }
}

void loadAll() {
    loadUsers();
    loadNotices();
    loadComplaints();
    if (db.users.empty()) migrateLegacy();
}

}

namespace {

std::string readLine(const std::string& prompt) {
    std::cout << "  " << std::left << std::setw(24) << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << "\n";
        std::exit(0);
    }
    return trim(line);
}

std::string readSecret(const std::string& prompt) {
    std::cout << "  " << std::left << std::setw(24) << prompt << std::flush;
    term::setRaw();
    std::string secret;
    int c;
    while ((c = term::rawGet()) >= 0) {
        if (c == 13 || c == 10) break;
        if (c == 4) {
            term::restore();
            std::exit(0);
        }
        if (c == 127 || c == 8) {
            if (!secret.empty()) {
                secret.pop_back();
                std::cout << "\b \b";
                std::cout.flush();
            }
            continue;
        }
        if (c < 32 || secret.size() >= kPasswordMax) continue;
        secret.push_back(static_cast<char>(c));
        std::cout << '*';
        std::cout.flush();
    }
    term::restore();
    std::cout << "\n";
    return secret;
}

bool confirm(const std::string& question) {
    while (true) {
        std::cout << "  " << question << " [Y/n] " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) std::exit(0);
        std::string v = lower(trim(line));
        if (v.empty() || v == "y" || v == "yes") return true;
        if (v == "n" || v == "no") return false;
        ui::note("Please type Y or N.", ui::Red);
    }
}

int choose(const std::string& title, const std::vector<std::string>& items, int start = 0) {
    if (items.empty()) return -1;
    ui::header(title);
    int sel = std::min(std::max(start, 0), static_cast<int>(items.size()) - 1);
    bool redraw = false;
    auto draw = [&]() {
        for (std::size_t i = 0; i < items.size(); ++i) {
            bool active = static_cast<int>(i) == sel;
            std::cout << "   " << (active ? ui::set(ui::Cyan) + " > " + ui::set(ui::Reset) : "   ")
                      << (active ? ui::set(ui::Bold) : ui::set(ui::Reset)) << items[i] << ui::set(ui::Reset)
                      << "\n";
        }
    };
    int lines = static_cast<int>(items.size());
    ui::footer();
    std::cout << "\n";
    term::setRaw();
    struct RawGuard {
        ~RawGuard() { term::restore(); }
    } guard;
    for (;;) {
        if (redraw) std::cout << "\033[" << (lines + 1) << "A\033[J";
        redraw = true;
        draw();
        std::cout.flush();
        term::Key k = term::readKey();
        if (k == term::CtrlD) {
            ui::note("Input closed. Exiting.", ui::Red);
            std::exit(0);
        }
        if (k == term::Up) {
            sel = (sel - 1 + static_cast<int>(items.size())) % static_cast<int>(items.size());
        } else if (k == term::Down) {
            sel = (sel + 1) % static_cast<int>(items.size());
        } else if (k >= '1' && k <= '9') {
            int idx = k - '1';
            if (idx < static_cast<int>(items.size())) return idx;
        } else if (k == 10) {
            return sel;
        } else if (k == term::Esc) {
            ui::note("Cancelled.");
            return -1;
        }
    }
}

int askInt(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::string v = readLine(prompt);
        if (v.empty()) {
            ui::note("Enter a number between " + std::to_string(lo) + " and " + std::to_string(hi) + ".", ui::Red);
            continue;
        }
        bool ok = true;
        long parsed = 0;
        for (char c : v) {
            if (!std::isdigit(static_cast<unsigned char>(c))) { ok = false; break; }
            parsed = parsed * 10 + (c - '0');
            if (parsed > 1000000) { ok = false; break; }
        }
        if (!ok || parsed < lo || parsed > hi) {
            ui::note("Out of range. Try again.", ui::Red);
            continue;
        }
        return static_cast<int>(parsed);
    }
}

bool askPhone(const std::string& prompt, std::string& out) {
    while (true) {
        std::string v = readLine(prompt);
        std::string digits;
        for (char c : v) {
            if (std::isdigit(static_cast<unsigned char>(c))) digits.push_back(c);
        }
        if (digits.size() != 10) {
            ui::note("Enter a 10 digit phone number.", ui::Red);
            continue;
        }
        out = digits;
        return true;
    }
}

std::vector<int> freeSeats(const Route& r) {
    std::vector<int> free;
    for (int s = 1; s <= r.capacity; ++s) {
        bool taken = false;
        for (const User* u : app::usersOnRoute(r.id)) {
            if (u->seat == s) { taken = true; break; }
        }
        if (!taken) free.push_back(s);
    }
    return free;
}

void drawSeatMap(const Route& r, const std::string& mine) {
    std::cout << "\n";
    ui::note("Bus " + r.id + "  \u00b7  " + r.name + "  \u00b7  " + r.via, ui::Cyan);
    std::cout << "\n";
    for (int s = 1; s <= r.capacity; ++s) {
        std::string holder;
        for (const User* u : app::usersOnRoute(r.id)) {
            if (u->seat == s) { holder = u->username; break; }
        }
        std::string code;
        ui::Color col;
        if (!holder.empty() && holder == mine) {
            code = "[" + std::to_string(s) + "]";
            col = ui::Green;
        } else if (!holder.empty()) {
            code = "[ X ]";
            col = ui::Red;
        } else {
            code = "[ " + std::to_string(s) + "]";
            col = ui::Dim;
        }
        std::cout << "  " << ui::set(col) << code << ui::set(ui::Reset);
        if (s % 6 == 0) std::cout << "\n";
    }
    if (r.capacity % 6 != 0) std::cout << "\n";
    std::cout << "\n";
    ui::note("[ n ] free   [ X ] allotted   [ n ] yours", ui::Dim);
}

std::vector<int> complaintIdsFor(const std::string& username, bool openOnly) {
    std::vector<int> ids;
    for (const Complaint& c : app::db.complaints) {
        if (c.username != username) continue;
        if (openOnly && c.status == ComplaintStatus::Resolved) continue;
        ids.push_back(c.id);
    }
    return ids;
}

Complaint* findComplaint(int id) {
    for (Complaint& c : app::db.complaints) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

}

namespace screens {

void showNotices(bool staffMode, const std::string& author = "") {
    ui::banner();
    ui::header("NOTICE BOARD");
    if (app::db.notices.empty()) {
        ui::note("No notices have been published yet.", ui::Yellow);
    } else {
        for (auto it = app::db.notices.rbegin(); it != app::db.notices.rend(); ++it) {
            std::cout << "\n  " << ui::set(ui::Bold) << it->text << ui::set(ui::Reset) << "\n";
            std::cout << "  " << ui::set(ui::Dim) << "posted " << it->stamp << " by " << it->author
                      << ui::set(ui::Reset) << "\n";
        }
    }
    if (staffMode) {
        if (!confirm("Publish a new notice now?")) return;
        ui::note("Type the notice, then press Enter on an empty line to finish.", ui::Cyan);
        std::string text;
        while (std::getline(std::cin, text)) {
            if (trim(text).empty()) break;
            if (!text.empty()) text += "\n";
        }
        text = trim(text);
        if (text.empty()) {
            ui::note("Notice was empty. Nothing saved.", ui::Red);
            ui::pause();
            return;
        }
        Notice n;
        n.stamp = nowStamp();
        n.author = author;
        n.text = text;
        app::db.notices.push_back(n);
        app::saveNotices();
        ui::note("Notice published.", ui::Green);
    }
    ui::pause();
}

void showRoutes() {
    ui::banner();
    ui::header("ROUTES  &  FARES");
    std::cout << "\n";
    std::cout << "  " << ui::set(ui::Bold) << ui::pad("  ROUTE", 9) << ui::pad("PATH", 34)
              << ui::pad("ANNUAL FARE", 14) << ui::pad("SEATS", 8) << "STAFF" << ui::set(ui::Reset)
              << "\n";
    ui::rule();
    for (const Route& r : routes()) {
        std::string staff = "\u2014";
        for (const User* u : app::usersOnRoute(r.id)) {
            (void)u;
        }
        for (const User& u : app::db.users) {
            if (u.role == Role::Staff && u.routeId == r.id) {
                staff = u.username;
                break;
            }
        }
        std::string free = std::to_string(freeSeats(r).size()) + " free";
        std::cout << "  " << ui::pad("  " + r.id, 9) << ui::pad(r.name, 34)
                  << ui::pad("Rs " + money(r.fare), 14) << ui::pad(free, 8)
                  << (staff == "\u2014" ? ui::set(ui::Dim) : ui::set(ui::Green)) << staff
                  << ui::set(ui::Reset) << "\n";
    }
    ui::rule();
    ui::note("Registration fee of Rs " + money(kRegistrationFee) + " applies to every new student.", ui::Dim);
    ui::pause();
}

void registration() {
    ui::banner();
    ui::header("REGISTRATION");
    int rolePick = choose("I am registering as", {"Student", "Staff"}, 0);
    if (rolePick < 0) return;
    Role role = rolePick == 0 ? Role::Student : Role::Staff;

    std::string suggested = app::nextUsername(role);

    ui::banner();
    ui::header(role == Role::Student ? "STUDENT REGISTRATION" : "STAFF REGISTRATION");
    ui::kv("Account type", roleName(role));
    ui::note("Fields marked * are required. Press Enter to accept the suggestion in brackets.", ui::Dim);
    std::cout << "\n";

    std::string name;
    do {
        name = readLine("Full name *");
        if (name.empty()) ui::note("Name cannot be empty.", ui::Red);
    } while (name.empty());

    std::string father;
    do {
        father = readLine("Father / Guardian *");
        if (father.empty()) ui::note("Guardian name cannot be empty.", ui::Red);
    } while (father.empty());

    std::string phone;
    askPhone("Mobile number *", phone);

    std::string address = readLine("Address");
    if (address.empty()) address = "-";

    std::string username;
    while (true) {
        std::string typed = readLine("Username [" + suggested + "]");
        username = typed.empty() ? suggested : typed;
        if (username.find_first_of(" |%") != std::string::npos) {
            ui::note("Username cannot contain spaces or '|'.", ui::Red);
            continue;
        }
        if (username.size() < 3 || username.size() > 20) {
            ui::note("Username must be 3 to 20 characters.", ui::Red);
            continue;
        }
        if (app::usernameTaken(username)) {
            ui::note("That username is already taken. Pick another.", ui::Red);
            continue;
        }
        break;
    }

    std::string secret;
    while (true) {
        secret = readSecret("Password *");
        if (secret.size() < kPasswordMin) {
            ui::note("Password must be at least " + std::to_string(kPasswordMin) + " characters.", ui::Red);
            continue;
        }
        std::string again = readSecret("Confirm password *");
        if (again != secret) {
            ui::note("Passwords did not match. Try again.", ui::Red);
            continue;
        }
        break;
    }

    User u;
    u.username = username;
    u.role = role;
    u.name = name;
    u.father = father;
    u.phone = phone;
    u.address = address;
    makeSaltAndHash(secret, u.salt, u.hash);

    if (role == Role::Student) {
        std::vector<std::string> routeItems;
        for (const Route& r : routes()) routeItems.push_back(r.id + "  " + r.name);
        int pick = choose("Choose your route", routeItems, 0);
        if (pick < 0) {
            ui::note("Registration cancelled.", ui::Yellow);
            ui::pause();
            return;
        }
        const Route& r = routes()[static_cast<std::size_t>(pick)];
        u.routeId = r.id;
        u.due = r.fare;
        u.paid = false;

        std::vector<int> free = freeSeats(r);
        if (free.empty()) {
            ui::note("Route " + r.id + " is full. Registration cancelled.", ui::Red);
            ui::pause();
            return;
        }
        drawSeatMap(r, "");
        std::vector<std::string> seatItems;
        for (int s : free) seatItems.push_back("Seat " + std::to_string(s));
        int seatPick = choose("Select your seat", seatItems, 0);
        if (seatPick < 0) {
            ui::note("Registration cancelled.", ui::Yellow);
            ui::pause();
            return;
        }
        u.seat = free[static_cast<std::size_t>(seatPick)];
    }

    std::cout << "\n";
    ui::header("CONFIRM YOUR DETAILS");
    ui::kv("Username", u.username);
    ui::kv("Role", roleName(u.role));
    ui::kv("Name", u.name);
    ui::kv("Guardian", u.father);
    ui::kv("Mobile", u.phone);
    ui::kv("Address", u.address);
    if (u.role == Role::Student) {
        ui::kv("Route", routeLabel(u.routeId));
        ui::kv("Seat number", std::to_string(u.seat));
        ui::kv("Payable now", "Rs " + money(u.totalPayable()));
    } else {
        ui::kv("Route allotment", "Assigned by the Transport Cell");
    }
    std::cout << "\n";
    if (!confirm("Confirm and create this account?")) {
        ui::note("Registration discarded.", ui::Yellow);
        ui::pause();
        return;
    }

    app::db.users.push_back(u);
    app::saveUsers();

    ui::banner();
    ui::header("REGISTRATION SUCCESSFUL");
    ui::note("Account created.", ui::Green);
    ui::kv("Username", u.username);
    if (u.role == Role::Student) {
        ui::kv("Route", routeLabel(u.routeId));
        ui::kv("Seat number", std::to_string(u.seat));
        ui::kv("Amount payable", "Rs " + money(u.totalPayable()));
        ui::note("Please pay at the transport office on or before the 5th of July.", ui::Yellow);
    }
    std::cout << "\n";
    ui::note("Sign in with the username above to continue.", ui::Cyan);
    ui::pause();
}

bool authenticate(const std::string& username, const std::string& secret, Role expected) {
    const User* u = app::findUserConst(username);
    if (!u) return false;
    if (u->role != expected) return false;
    return verifySecret(*u, secret);
}

void allotmentCard(const User& u) {
    ui::banner();
    ui::header("MY ALLOTMENT CARD");
    std::cout << "\n";
    ui::kv("Student", u.name);
    ui::kv("Guardian", u.father);
    ui::kv("Mobile", u.phone);
    ui::kv("Address", u.address);
    ui::kv("Route", routeLabel(u.routeId));
    ui::kv("Seat number", u.seat > 0 ? std::to_string(u.seat) : "-");
    const Route* r = findRoute(u.routeId);
    ui::kv("Annual fare", r ? "Rs " + money(r->fare) : "-");
    ui::kv("Registration fee", "Rs " + money(kRegistrationFee));
    std::cout << "\n";
    ui::rule();
    if (u.paid || u.due <= 0) {
        ui::note("No dues pending. Transport fee is fully paid.", ui::Green);
    } else {
        std::cout << "  " << ui::set(ui::Bold) << "AMOUNT DUE : Rs " << money(u.due) << ui::set(ui::Reset)
                  << "\n";
    }
    std::cout << "\n";
    if (r) drawSeatMap(*r, u.username);
    ui::note("This card must be shown to the driver at boarding.", ui::Dim);
    ui::pause();
}

void payDues(User& u) {
    ui::banner();
    ui::header("PAY TRANSPORT DUES");
    const Route* r = findRoute(u.routeId);
    std::cout << "\n";
    ui::kv("Student", u.name + " (" + u.username + ")");
    ui::kv("Route", routeLabel(u.routeId));
    ui::kv("Seat number", std::to_string(u.seat));
    ui::kv("Annual fare", r ? "Rs " + money(r->fare) : "-");
    ui::kv("Total pending", "Rs " + money(u.totalPayable()));
    std::cout << "\n";
    if (u.paid || u.totalPayable() <= 0) {
        ui::note("Nothing to pay. Your account is clear.", ui::Green);
        ui::pause();
        return;
    }
    if (!confirm("Record a payment towards your dues?")) return;
    int amount = askInt("Amount paid (Rs) 1 - " + std::to_string(u.totalPayable()), 1,
                        static_cast<int>(u.totalPayable()));
    long long paid = amount;
    if (paid > u.due) {
        paid = u.due;
        ui::note("Capping payment at the outstanding fare of Rs " + money(u.due) + ".", ui::Yellow);
    }
    u.due -= paid;
    if (u.due <= 0) {
        u.due = 0;
        u.paid = true;
    }
    app::saveUsers();
    ui::note("Rs " + money(paid) + " recorded. Remaining balance: Rs " + money(u.totalPayable()),
             ui::Green);
    ui::pause();
}

void raiseComplaint(User& u) {
    ui::banner();
    ui::header("RAISE A COMPLAINT");
    ui::note("Describe the issue. End with an empty line.", ui::Cyan);
    std::cout << "\n";
    std::string text;
    while (std::getline(std::cin, text)) {
        if (trim(text).empty()) break;
        if (!text.empty()) text += "\n";
    }
    text = trim(text);
    if (text.empty()) {
        ui::note("Nothing was entered. Complaint not filed.", ui::Red);
        ui::pause();
        return;
    }
    Complaint c;
    c.id = app::nextComplaintId();
    c.stamp = nowStamp();
    c.username = u.username;
    c.text = text;
    c.status = ComplaintStatus::Open;
    app::db.complaints.push_back(c);
    app::saveComplaints();
    ui::note("Complaint #" + std::to_string(c.id) + " filed on " + c.stamp + ".", ui::Green);
    ui::pause();
}

void myComplaints(const User& u) {
    ui::banner();
    ui::header("MY COMPLAINTS");
    std::vector<int> ids = complaintIdsFor(u.username, false);
    if (ids.empty()) {
        ui::note("You have not filed any complaints.", ui::Yellow);
        ui::pause();
        return;
    }
    for (int id : ids) {
        const Complaint* c = findComplaint(id);
        if (!c) continue;
        ui::Color col = c->status == ComplaintStatus::Open ? ui::Yellow : ui::Green;
        std::cout << "\n  " << ui::set(ui::Bold) << "#" << c->id << "  " << c->text << ui::set(ui::Reset)
                  << "\n";
        std::cout << "  " << ui::set(ui::Dim) << c->stamp << "   " << ui::set(col)
                  << statusName(c->status) << ui::set(ui::Reset) << "\n";
    }
    ui::pause();
}

void staffRoster() {
    ui::banner();
    ui::header("STUDENT ROSTER");
    std::vector<const User*> students;
    for (const User& u : app::db.users) {
        if (u.role == Role::Student) students.push_back(&u);
    }
    if (students.empty()) {
        ui::note("No students registered yet.", ui::Yellow);
        ui::pause();
        return;
    }
    std::sort(students.begin(), students.end(),
              [](const User* a, const User* b) { return a->username < b->username; });
    std::cout << "\n";
    std::cout << "  " << ui::set(ui::Bold) << ui::pad("USERNAME", 12) << ui::pad("NAME", 22)
              << ui::pad("ROUTE", 8) << ui::pad("SEAT", 7) << ui::pad("DUE", 12) << "STATUS"
              << ui::set(ui::Reset) << "\n";
    ui::rule();
    long long total = 0;
    for (const User* s : students) {
        total += s->totalPayable();
        std::cout << "  " << ui::pad(s->username, 12) << ui::pad(s->name, 22) << ui::pad(s->routeId, 8)
                  << ui::pad(std::to_string(s->seat), 7) << ui::pad(money(s->totalPayable()), 12)
                  << (s->paid ? ui::set(ui::Green) : ui::set(ui::Yellow))
                  << (s->paid ? "PAID" : "PENDING") << ui::set(ui::Reset) << "\n";
    }
    ui::rule();
    ui::kv("Students", std::to_string(students.size()));
    ui::kv("Outstanding", "Rs " + money(total));
    ui::pause();
}

void complaintBox(User& staff) {
    while (true) {
        std::vector<Complaint> open;
        for (const Complaint& c : app::db.complaints) {
            if (c.status == ComplaintStatus::Open) open.push_back(c);
        }
        std::vector<std::string> items;
        for (const Complaint& c : open) {
            std::string oneLine = c.text;
            std::size_t nl = oneLine.find('\n');
            if (nl != std::string::npos) oneLine = oneLine.substr(0, nl) + " ...";
            if (oneLine.size() > 52) oneLine = oneLine.substr(0, 52) + "...";
            items.push_back("#" + std::to_string(c.id) + "  [" + c.username + "]  " + oneLine);
        }
        items.push_back("Back");

        ui::banner();
        ui::header("COMPLAINT BOX");
        if (items.size() == 1) {
            ui::note("No open complaints.", ui::Green);
            ui::pause();
            return;
        }
        int pick = choose("Open complaints", items, 0);
        if (pick < 0 || pick == static_cast<int>(items.size()) - 1) return;
        if (pick < 0 || pick >= static_cast<int>(open.size())) return;

        const Complaint& c = open[static_cast<std::size_t>(pick)];
        ui::banner();
        ui::header("COMPLAINT #" + std::to_string(c.id));
        ui::kv("Filed by", c.username);
        ui::kv("Filed on", c.stamp);
        std::cout << "\n";
        ui::note(c.text, ui::White);
        std::cout << "\n";
        if (confirm("Mark this complaint as resolved?")) {
            Complaint* target = findComplaint(c.id);
            if (target) {
                target->status = ComplaintStatus::Resolved;
                app::saveComplaints();
                ui::note("Complaint #" + std::to_string(c.id) + " marked resolved.", ui::Green);
            }
            (void)staff;
        }
    }
}

void allotStaffRoute(User& staff) {
    while (true) {
        ui::banner();
        ui::header("ROUTE STAFF ALLOTMENT");
        std::vector<std::string> items;
        for (const Route& r : routes()) {
            std::string who = "unallotted";
            for (const User& u : app::db.users) {
                if (u.role == Role::Staff && u.routeId == r.id) {
                    who = u.username;
                    break;
                }
            }
            items.push_back(r.id + "  " + r.name + "   \u2192  " + who);
        }
        int pick = choose("Select a route", items, 0);
        if (pick < 0) return;
        const Route& r = routes()[static_cast<std::size_t>(pick)];

        ui::banner();
        ui::header("ALLOT STAFF TO " + r.id);
        ui::kv("Route", r.name);
        ui::kv("Via", r.via);
        ui::kv("Seats", std::to_string(r.capacity));
        ui::kv("Students", std::to_string(app::usersOnRoute(r.id).size()));
        ui::kv("Your username", staff.username);
        std::cout << "\n";
        if (!confirm("Take allotment for route " + r.id + "?")) continue;

        for (User& u : app::db.users) {
            if (u.role == Role::Staff && u.routeId == r.id && u.username != staff.username) {
                u.routeId.clear();
            }
        }
        staff.routeId = r.id;
        app::saveUsers();
        ui::note("You are now the allotted staff for route " + r.id + ".", ui::Green);
        ui::pause();
    }
}

void staffMenu(User& staff) {
    while (true) {
        std::vector<std::string> items = {
            "Complaint Box",
            "Publish / view notices",
            "Route staff allotment",
            "Student roster and dues",
            "Routes and fares",
            "Logout",
        };
        ui::banner();
        std::cout << "  " << ui::set(ui::Green) << "STAFF CONSOLE" << ui::set(ui::Reset) << "   signed in as "
                  << ui::set(ui::Bold) << staff.username << ui::set(ui::Reset) << "\n";
        if (!staff.routeId.empty()) ui::note("Allotted to route " + staff.routeId, ui::Cyan);
        int pick = choose("Staff menu", items, 0);
        if (pick < 0 || pick == 5) {
            ui::note("Logged out. Goodbye, " + staff.username + ".");
            return;
        }
        switch (pick) {
            case 0: complaintBox(staff); break;
            case 1: showNotices(true, staff.username); break;
            case 2: allotStaffRoute(staff); break;
            case 3: staffRoster(); break;
            case 4: showRoutes(); break;
            default: break;
        }
    }
}

void studentMenu(User& student) {
    while (true) {
        std::vector<std::string> items = {
            "My allotment card",
            "Pay transport dues",
            "Raise a complaint",
            "My complaints",
            "Routes and fares",
            "Transport helpline",
            "Logout",
        };
        ui::banner();
        std::cout << "  " << ui::set(ui::Cyan) << "STUDENT CONSOLE" << ui::set(ui::Reset) << "   signed in as "
                  << ui::set(ui::Bold) << student.username << ui::set(ui::Reset) << "\n";
        if (!student.paid && student.totalPayable() > 0) {
            ui::note("Dues pending: Rs " + money(student.totalPayable()), ui::Yellow);
        }
        int pick = choose("Student menu", items, 0);
        if (pick < 0 || pick == 6) {
            ui::note("Logged out. Goodbye, " + student.username + ".");
            return;
        }
        User* live = app::findUser(student.username);
        if (!live) {
            ui::note("Your account could not be found.", ui::Red);
            return;
        }
        student = *live;
        switch (pick) {
            case 0: allotmentCard(student); break;
            case 1: payDues(student); break;
            case 2: raiseComplaint(student); break;
            case 3: myComplaints(student); break;
            case 4: showRoutes(); break;
            case 5: {
                ui::banner();
                ui::header("TRANSPORT HELPLINE");
                ui::kv("Transport In-charge", "Dr. B. Sreenivasa Reddy");
                ui::kv("Designation", "Associate Professor, Dept. of Physics");
                ui::kv("Room", "K-201, CBIT, Hyderabad");
                ui::kv("Phone", "9832333393");
                ui::kv("Office hours", "09:00 - 16:00, Mon to Sat");
                std::cout << "\n";
                ui::pause();
                break;
            }
            default: break;
        }
    }
}

void login(Role role) {
    std::string label = role == Role::Student ? "STUDENT" : "STAFF";
    ui::banner();
    ui::header(label + " LOGIN");
    std::string username;
    while (true) {
        username = readLine("Username");
        if (!username.empty()) break;
        ui::note("Username is required.", ui::Red);
    }
    std::string secret;
    while (true) {
        secret = readSecret("Password");
        if (!secret.empty()) break;
        ui::note("Password is required.", ui::Red);
    }

    if (!authenticate(username, secret, role)) {
        const User* exists = app::findUserConst(username);
        ui::banner();
        ui::note("Login failed.", ui::Red);
        if (exists && exists->role != role) {
            ui::note("That account is registered as " +
                         std::string(roleName(exists->role)) + ", not " + label + ".",
                     ui::Yellow);
        } else {
            ui::note("Check the username and password and try again.", ui::Yellow);
        }
        ui::pause();
        return;
    }

    User* u = app::findUser(username);
    ui::banner();
    ui::note("Welcome back, " + u->name + "!", ui::Green);
    ui::pause();
    if (u->role == Role::Staff) {
        staffMenu(*u);
    } else {
        studentMenu(*u);
    }
}

}


using namespace screens;

int main() {
    std::ios::sync_with_stdio(false);
    term::init();

    if (std::system("mkdir -p data 2>/dev/null") != 0) { /* handled below */ }
    app::loadAll();

    if (app::db.users.empty()) {
        ui::banner();
        ui::header("WELCOME");
        ui::note("No accounts exist yet. Register to get started.", ui::Cyan);
        ui::note("Demo accounts are not created automatically; registration is open to everyone.", ui::Dim);
        ui::pause();
    }

    while (true) {
        std::vector<std::string> items = {
            "Register now",
            "Student login",
            "Staff login",
            "Notice board",
            "Routes and fares",
            "Exit",
        };
        ui::banner();
        std::cout << "  " << ui::set(ui::Dim)
                  << "Seats are allotted per route. Fees, complaints and staff routing are tracked here."
                  << ui::set(ui::Reset) << "\n";
        int pick = choose("Main menu", items, 0);
        if (pick < 0 || pick == 5) {
            ui::banner();
            ui::note("Thanks for using the transport portal. Goodbye.", ui::Cyan);
            app::saveAll();
            return 0;
        }
        switch (pick) {
            case 0: registration(); break;
            case 1: login(Role::Student); break;
            case 2: login(Role::Staff); break;
            case 3: showNotices(false); break;
            case 4: showRoutes(); break;
            default: break;
        }
    }
}
