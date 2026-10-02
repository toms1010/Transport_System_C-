# Student Transport Management System

A transport management application for a college transport cell, written in
C++17 with a layered architecture and CSV persistence. It ships with two front
ends over one shared domain layer: a console application and a Qt Widgets
desktop application.

Students take a seat on a route, see exactly what they owe, pay against either
fee bucket, and track their complaints to resolution. Staff manage the roster,
routes, seats, dues, complaints and notices. An administrator oversees all of it.

```
┌────────────────────────────────────────────────────────────────────────────┐
│ │                        FRONT ENDS (interchangeable)                      │ │
│ │  Console (src/ui, transport_system)   Qt Widgets (gui/, transport_gui)   │ │
│ └────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                        │
┌────────────────────────────────────────────────────────────────────────────┐
│                             SERVICE LAYER                                 │
│  AuthService · RouteService · SeatService · PaymentService                │
│  ComplaintService · NoticeService · ConfigService                         │
└────────────────────────────────────────────────────────────────────────────┘
                                    │
┌────────────────────────────────────────────────────────────────────────────┐
│                           REPOSITORY LAYER                                │
│  UserRepository · RouteRepository · PaymentRepository                     │
│  ComplaintRepository · NoticeRepository                                   │
└────────────────────────────────────────────────────────────────────────────┘
                                    │
┌────────────────────────────────────────────────────────────────────────────┐
│                              DATA LAYER                                   │
│  data/users.csv · routes.csv · payments.csv · complaints.csv · notices.csv │
└────────────────────────────────────────────────────────────────────────────┘
```

## Editor setup (VS Code)

The workspace ships `.vscode/` configuration so IntelliSense resolves the
project's headers without guessing:

- `includePath` points at `include/` and `tests/` explicitly. A recursive
  `${workspaceFolder}/**` does not reliably resolve quoted project includes like
  `"models/User.hpp"` and drags the whole `build/` tree into the index.
- `cppStandard` is `c++17` (required for `std::filesystem`, `if constexpr`,
  `make_unique`).
- `compile_commands.json` is exported by CMake and consumed directly, so the
  editor sees the exact flags the project builds with.
- Tasks wrap CMake: **CMake: Configure / Build / Test**. Launch configurations
  cover `transport_system` and every test suite.

> **Disable the "C/C++ Runner" extension** (`ms-vscode-2019.crunner`). It is
> built for the old single-file `g++ main.cpp` flow, it compiles one file at a
> time (which cannot link this project), and it keeps re-inserting its own
> `C/C++ Runner: Debug Session` entry into `.vscode/launch.json`. Its settings
> have been removed; reload the window once (`Ctrl+Shift+P` → *Developer: Reload
> Window*) to let the change take effect.

## Build

Requires CMake 3.16+ and a C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+).

```bash
cmake -S . -B build
cmake --build build
./build/bin/transport_system
```

Release build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The build is warning-free under `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion`.

## Desktop GUI

The same system is also available as a Qt Widgets desktop application. It reuses
the domain layer unchanged — no business logic is duplicated between the two front
ends — so both read and write the same `data/*.csv` files.

```bash
sudo apt install qt6-base-dev      # or your distro's Qt 6 package
cmake -S . -B build
cmake --build build
./build/bin/transport_gui
```

Qt 6.2+ is required (`Qt6::Widgets` and `Qt6::PrintSupport`). If Qt is not
installed, CMake skips the `transport_gui` target and prints a note instead of
failing, so the console build still works. To skip it deliberately:

```bash
cmake -S . -B build -DST_BUILD_GUI=OFF
```

The GUI resolves its data folder from `ST_DATA_DIR`, then `./data`, then
`../data`. Override it to run against a separate dataset:

```bash
ST_DATA_DIR=/path/to/data ./build/bin/transport_gui
```

First run creates `data/` and an administrator account (`admin` / `admin123`).
Accounts created in either front end are visible in the other immediately.

In VS Code: **Run desktop GUI** (tasks) or **Debug transport_gui** (F5).

### What the GUI covers

| Page | Role | Does |
|------|------|------|
| Public window | none | Routes, notice board and about, readable before signing in |
| Login | — | Sign in, show/hide password, forgot-password guidance, registration, links to the public pages |
| Registration wizard | — | Four steps: personal details, credentials, route and seat, summary. The account is written only on the final step |
| Dashboard | all | Route, seat and payment cards for students; students, routes, complaints, occupancy and dues for staff |
| My transport / Seat map | all | Interactive seat grid. Students claim and release their own seat; staff assign, auto-assign or release for a selected student |
| Students | staff, admin | Roster with search and route/fee filters; hands off to the seat map or payments page |
| Routes | staff, admin | Route CRUD, activate/deactivate, occupancy and free-seat counts |
| Payments & dues | all | Per-bucket statement with progress bars, record or settle a payment, receipt history, CSV export |
| Complaints / My complaints | all | Students submit and track; staff read, resolve with a written response, or reopen |
| Notices | all | Notice board; staff and admin publish, edit and delete |
| Allotment card | student | Printable transport card showing route, seat and fee position |
| Staff accounts | admin | Create staff accounts, assign a route, activate/deactivate, reset passwords |
| Reports | staff, admin | Financial summary, per-route utilisation, students and collections by route, complaint totals, ledger export |
| My profile | all | View and edit own details, change password |
| Help & contact | all | Task-by-task guidance and transport office details |
| Settings | all | Light/dark theme, data folder, backup, restore |

### Front-end architecture

```
GUI widgets → Controllers → Services → Repositories → data/*.csv
```

No widget calls a service directly. The controller layer
(`include/controllers/`, `gui/controllers/`) turns model data into display-ready
rows and keeps validation messages in one place, so the dashboard, the payments
page and the reports page can never disagree about a balance. See
[docs/gui-design.md](docs/gui-design.md).

### Theming

One stylesheet, `resources/styles/app.qss`, with `{{TOKEN}}` placeholders that
`Theme.cpp` substitutes from the active palette. Light and dark share the same
rules and no widget hardcodes a colour. 31 SVG icons ship in
`resources/icons/` via `resources.qrc`; nothing is fetched at runtime. The
theme is persisted in `QSettings` and switchable from the top bar or Settings.

## Test

69 tests across six binaries, registered with CTest.

```bash
ctest --test-dir build --output-on-failure
```

Or run one suite directly:

```bash
./build/bin/test_payments
```

| Suite | Covers |
|-------|--------|
| `test_auth` | registration, login, wrong password, duplicate username, password policy, no plaintext on disk, staff authorization, password change, deactivated accounts |
| `test_routes` | default route seeding, lookup, add/update/delete, duplicate and invalid route rejection, delete-guard |
| `test_seats` | free/occupied seats, duplicate seat prevention, out-of-range seats, release, route change, rollback on failure |
| `test_payments` | totals, partial payment, overpayment capping, independent buckets, full settlement, invalid amounts, history, persistence, aggregates |
| `test_complaints` | complaint creation, validation, resolve, reopen, filtering, persistence, id sequencing, notice lifecycle |
| `test_robustness` | missing files, truncated CSV rows, non-numeric numeric fields, oversized text, `|`/`%` round-tripping, strict numeric parsing, money formatting, hostile input, duplicate seats in a hand-edited file |

## First run

On first start the application creates `data/` and, if no accounts exist, an
administrator account:

```
Username   admin
Password   admin123
```

Change this immediately. If pre-refactor files (`list_of_students`,
`login.txt`) are present in the working directory they are imported once and
the result is recorded in `data/migration.log`; those accounts use the
placeholder secret `legacy` and should be reset.

## Project layout

```
StudentTransport/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── models/        User, Student, Staff, Admin, Route, Seat,
│   │                  Payment, Complaint, Notice, FeeBreakdown
│   ├── services/      AuthService, RouteService, SeatService, PaymentService,
│   │                  ComplaintService, UserService (ConfigService)
│   ├── repositories/  UserRepository, PaymentRepository, ComplaintRepository
│   ├── ui/            ConsoleUI, Menu, Screens
│   └── utils/         TextUtils, CsvUtils, FileUtils, Security, Validation
├── src/               mirrors include/
├── include/controllers/  GUI controller layer (no Qt-free domain types)
│   ├── ActionResult.hpp      shared success/failure result
│   ├── AuthController.*      login, registration, password, profile
│   ├── RouteController.*     route rows, options, CRUD
│   ├── SeatController.*      seat map, claim/assign/release
│   ├── PaymentController.*   statement, history, aggregates, export
│   ├── ComplaintController.* submit, resolve, reopen, filters
│   ├── NoticeController.*    publish, edit, delete
│   ├── StudentController.*   roster, filters, profiles
│   ├── StaffController.*     accounts, route assignment, passwords
│   └── Controllers.hpp       the bundle every page receives
├── gui/               Qt Widgets desktop front end
│   ├── main.cpp       entry point: login loop and window handover
│   ├── AppContext.*   service container + first-run bootstrap + backup
│   ├── Theme.*        palettes, token substitution, icon loader
│   ├── Notify.*       toasts and the busy overlay
│   ├── Widgets.*      Page, Card, StatCard, PageHeader, Avatar, helpers
│   ├── LoginWindow.*  sign in
│   ├── RegistrationWizard.*  four-step student and staff sign-up
│   ├── PublicWindow.* routes / notices / about before sign-in
│   ├── Pickers.*      reusable route and seat choosers
│   ├── MainWindow.*   role-aware sidebar navigation
│   └── <Feature>Page.*   Dashboard, Routes, SeatMap, Payments, Roster,
│                        Complaints, Notices, Profile, StaffAccounts,
│                        Reports, Settings, Help, Allotment
├── resources/
│   ├── resources.qrc
│   ├── styles/app.qss tokenized stylesheet shared by both themes
│   └── icons/         31 inline SVG icons
├── data/              users.csv, routes.csv, payments.csv,
│                      complaints.csv, notices.csv, config.csv, migration.log
├── tests/             TestHarness + 5 suites
└── docs/              system-design.md, architecture.md, gui-design.md,
                       wireframes.md
```

## How the two front ends fit together

The build produces two static libraries so that neither front end can
accidentally depend on the other:

```
                 transport_domain        models, repositories, services,
                        |                utils — no console code, no Qt
            +-----------+-----------+
            |                       |
     transport_core           transport_gui
  (domain + src/ui/*.cpp)   (domain + controllers + Qt Widgets)
            |                       |
    transport_system          desktop application
  (the console app)
```

`src/ui/` is the only console-specific code in the project, so keeping it out of
`transport_domain` is what lets the GUI reuse the tested logic verbatim.

The GUI adds a controller layer between its widgets and the services. Pages
receive a `Controllers` bundle and read display-ready rows from it, so no widget
touches a service directly and no rule is implemented twice. See
[docs/gui-design.md](docs/gui-design.md).

## Features

**Students** — dashboard, profile, route selection, seat map, allotment card,
per-bucket payments with receipts, payment history, complaints, complaint status
with staff responses, notice board, password change, logout.

**Staff** — dashboard, self-assignment to a route, student roster, route CRUD,
seat allocation tools, per-student and aggregate dues, complaint queue with
resolve/reopen, notice publish/edit/delete, reports, logout.

**Admin** — everything staff can do plus staff account oversight.

Both front ends implement the same feature set. The console version is the
reference for keyboard-driven use; the desktop version adds search and filtering,
CSV export, printable cards, light and dark themes, and data backup and
restore.

## Design decisions worth knowing

**Payments use two balances, not one.** The original stored a single `due` plus
a fixed registration fee and capped payments against `due` while the total
re-added the fee — so the full amount could never be paid. Now
`registrationFee` and `transportFee` are separate, a payment targets one bucket
and is capped at that bucket's remaining balance, and staff reports read the
same balances the student sees.

**Seat allocation lives in one service.** `SeatService` is the only writer of a
student's seat, which is what guarantees "one seat, one student". Changing
route releases the old seat and rolls back if the new one cannot be taken.

**Users are held polymorphically.** `std::vector<std::unique_ptr<User>>` — a
`vector<User>` would slice `Student`/`Staff` back to `User` and make every
downcast undefined.

**Staff registration needs a real secret.** The authorization code lives in
`data/config.csv` and is compared in constant time, rather than accepting any
non-empty string. It is a shared secret, not identity management — the
limitation is documented rather than papered over.

**Password hashing is honest about its limits.** It is a salted, iterated digest
behind `utils/Security.hpp`, with constant-time comparison. It is not a
production KDF; swapping in Argon2 or bcrypt touches one file.

**Storage is replaceable.** Everything above the repository layer is unaware
that CSV is used. Moving to SQLite means reimplementing five classes.

## Documentation

- [docs/system-design.md](docs/system-design.md) — actors, numbered functional
  and non-functional requirements, workflow, data design
- [docs/architecture.md](docs/architecture.md) — layers, models, services,
  repositories, dependency graph, security and portability notes
- [docs/gui-design.md](docs/gui-design.md) — desktop front end: controller
  layer, navigation, theming, notifications, accessibility
- [docs/wireframes.md](docs/wireframes.md) — text wireframes for every screen,
  console and desktop

## Terminal layout

The menu owns its screen and repaints in place using absolute row addressing, so
navigation stays correct on terminals too short to hold the list. Widths come
from `contentWidth()`; long text is trimmed rather than wrapping, the key hint
drops its optional segments on narrow screens, and the landing screen carries a
footer. Verified down to 44x20.

## Keyboard

| Key | Action |
|-----|--------|
| Up / Down | move the selection |
| Enter | select |
| 1-9 | jump straight to an item |
| Esc | go back |
| Y / N | confirm a prompt |