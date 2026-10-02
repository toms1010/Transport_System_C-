# Architecture

## Layers

```
Presentation  ->  Service  ->  Repository  ->  Storage
 (ConsoleUI,      (Auth,       (Csv*Repo)      data/*.csv
  Menu,           Route,       )
  Screens)        Seat,
                  Payment,
                  Complaint,
                  Notice,
                  Config)
```

Dependencies point in one direction only. The UI never reads a file, and no
service knows the CSV layout.

## Models (`include/models`)

Plain data holders with behaviour that belongs to the entity, and no I/O.

| Type | Responsibility |
|------|----------------|
| `User` | Base account: identity, contact, credentials, role. Abstract (`clone()` is pure virtual). |
| `Student` | A `User` that is enrolled in transport. Owns route, seat and the two fee buckets. |
| `Staff` | A `User` with an assigned route. |
| `Admin` | A `User` with system-wide privileges. |
| `Route` | Path, via points, fare, capacity. |
| `Seat` | (routeId, seatNumber, studentId) plus derived `AVAILABLE/OCCUPIED/SELECTED` state. |
| `Payment` | One immutable money movement. |
| `Complaint` | Subject, description, status, staff response. |
| `Notice` | Title, body, author. |
| `FeeBreakdown` | The registration/transport split and the derived totals. |
| `SystemConfig` | Fees and contact details, loaded from disk rather than compiled in. |

### Polymorphic ownership

Users are stored as `std::vector<std::unique_ptr<User>>`. Storing them by value
in a `vector<User>` would slice `Student` and `Staff` back to `User`, after
which every downcast is undefined behaviour. `clone()` on each subclass makes
copy-into-cache safe.

## Repositories (`include/repositories`)

All storage access. Each repository owns one CSV file, escapes `|`, `%`, `\n`
and `\r`, writes atomically via a temp file plus rename, and tolerates malformed
rows by skipping them.

- `CsvUserRepository`
- `CsvRouteRepository` (also seeds the five default routes on first run)
- `CsvPaymentRepository`
- `CsvComplaintRepository`
- `CsvNoticeRepository`

To move to SQLite, reimplement these classes against a `sqlite3` handle. No
service or UI code changes, because nothing above this layer sees a file path.

## Services (`include/services`)

Business rules. Each service owns an in-memory cache loaded at start-up and
written back on mutation.

| Service | Rules it owns |
|---------|---------------|
| `AuthService` | credential verification, registration rules, role checks, session creation, balance rehydration from the payment journal |
| `RouteService` | route CRUD, capacity floor, occupancy, free-seat computation |
| `SeatService` | the ONLY writer of a student's seat; duplicate prevention; atomic route changes with rollback |
| `PaymentService` | per-bucket payment application, capping, receipts, aggregates |
| `ComplaintService` | complaint lifecycle and staff responses |
| `NoticeService` | notice lifecycle |
| `UserService` (`ConfigService`) | `data/config.csv` load/save, incl. fees, contact details and the staff authorization code |

### Seat ownership invariant

Seat state lives on the `Student` record (`routeId` + `seatNumber`). `SeatService`
is the single place allowed to change it. This is the fix for the original
code, which wrote seat data from several independent call sites and could hand
the same seat to two students.

`changeRoute()` releases the old seat, applies the new one, and rolls the student
back to the original seat if any step fails.

### Payment rehydration

`Student` stores its running balances, and `AuthService::findStudent()` rebuilds
them from the payment journal. The journal is therefore the source of truth and
the cached totals cannot drift from it.

`AuthService` keeps one in-memory snapshot of the journal. `PaymentService`
calls `AuthService::onJournalChanged()` immediately after appending, so balances
are correct on the very next read. (Without that notification every total
silently reverted to its pre-payment value.) Balances are also clamped to the
current fees, because a journal can outlive a fare change.

### Single user cache

`AuthService` is the only owner of user records. `RouteService` reads through
`auth.userCache()` instead of keeping a second copy - two caches previously let
seat/route state and fee balances drift apart.

Because a save replaces the cached object, `RouteService` exposes
`studentIdsOnRoute()` / `studentCountOnRoute()` (ids and counts) rather than
`Student*`. A pointer into the cache would dangle as soon as any save ran.

### Numeric input

`atoi`/`atoll` return 0 for non-numeric text and overflow to a negative number
past `int` range. Every numeric field from a file or a form goes through
`util::parseWholeNumber` / `util::parseIntInRange`, which reject rather than
coerce.

## UI (`include/ui`)

- `ConsoleUI` — platform-neutral terminal primitives: raw mode, key decoding,
  sizing, colour, masked input, multiline input.
- `Menu` — banners, rules, fields, tables, status tags, the bottom footer, and
  the arrow-key menu (which owns and repaints its own screen).
- `Screens` — the student, staff and admin flows; owns presentation only.

## Utilities (`include/utils`)

- `TextUtils` — trim/case, split/join, money formatting, timestamps.
- `CsvUtils` — field escaping and joining.
- `FileUtils` — line read/write/append, atomic write, directory creation.
- `Security` — password hashing, verification, constant-time comparison, policy.
- `Validation` — every validation rule, in one place.

## Dependencies

```
main.cpp
  └─ Screens ──┬── AuthService ──┬── CsvUserRepository
               ├── RouteService ─┴── CsvRouteRepository
               ├── SeatService
               ├── PaymentService ── CsvPaymentRepository
               ├── ComplaintService ─ CsvComplaintRepository
               └── NoticeService ─── CsvNoticeRepository
  All services ──── ConfigService ── data/config.csv
```

Services reference repositories; repositories never reference services. Services
receive each other by reference in `main.cpp`, so there is no service locator and
no hidden global state.

## Security notes

- Passwords are stored as a per-user salted digest. **This is not a production
  KDF.** `Security` is deliberately isolated so Argon2, bcrypt, scrypt or PBKDF2
  can replace the implementation in a single file. See the header comment.
- Digest comparison is constant time.
- Hashes and salts are never printed by any screen.
- Staff registration requires an authorization code, so a student cannot grant
  themselves elevated privileges by choosing a different role at the form.
- Accounts are deactivated through an `active` flag rather than deleted, which
  preserves the complaint and payment history that references them.

## Terminal layout

`menu()` owns the entire screen while it is open. It clears, paints the title,
rule, an optional context block, the key hint, the item list and a footer, then
repaints all of it in place on every keypress.

Two consequences, both deliberate:

- **Rows are addressed absolutely.** An earlier version moved the cursor back
  with `CSI <n> A` and cleared to end of screen. Once the drawn block scrolled
  past the top of a short terminal the cursor no longer sat below it, so each
  redraw left a duplicate copy of the menu behind. Absolute addressing cannot
  drift.
- **Anything a caller wants visible must be passed in.** `MenuOptions::context`
  carries the dashboard summary, because printing it before calling `menu()`
  would be erased by the first redraw.

All widths derive from `contentWidth()`, and box corners are counted inside it,
so no line ever soft-wraps. The key hint drops its optional segments rather than
being truncated, and long text is trimmed with a `~` marker.

## Cross-platform notes

- Directory creation uses `std::filesystem::create_directories`, not
  `system("mkdir -p data")`.
- Windows colour uses `ENABLE_VIRTUAL_TERMINAL_PROCESSING`; key reading uses
  `_getch` with the 0/224 extended-code prefix.
- POSIX raw mode uses `termios`; arrow keys arrive as `\033[A`-style sequences
  and are decoded with a short `select` timeout.
- `\n` and `\r` are both mapped to `Key::Enter`. The original code treated `\n`
  as "move down", so a pasted Enter silently changed the highlighted menu item.
---

# Desktop front end

The Qt Widgets application is a second front end over the same domain layer. It
adds no business rules; it moves the presentation and the user interaction.

```
   ┌─────────────────────────────┐        ┌─────────────────────────────┐
   │ console: src/ui/*.cpp       │        │ desktop: gui/*.cpp          │
   │ ConsoleUI · Menu · Screens  │        │ Windows · Pages · Dialogs   │
   └──────────────┬──────────────┘        └──────────────┬──────────────┘
                  │                                     │
                  │        ┌────────────────────────┐    │
                  └───────▶│   transport_domain    │◀───┘
                           │ models · repositories  │
                           │ services · utils       │
                           └───────────┬────────────┘
                                       │
                                  data/*.csv
```

## Build targets

| Target | Links | Purpose |
|--------|-------|---------|
| `transport_domain` | — | Models, repositories, services, utils. No console code, no Qt. |
| `transport_core` | `transport_domain` | Adds `src/ui/`, the console front end. |
| `transport_system` | `transport_core` | The console executable. |
| `transport_gui` | `transport_domain` + Qt 6 | The desktop executable. |
| `test_*` (6) | `transport_core` | The 69 tests. |

`transport_gui` deliberately does **not** link `transport_core`. If it did, the
desktop application would carry `iostream` and terminal code it never uses, and
the layering that keeps the domain reusable would stop meaning anything.

## The controller layer

`include/controllers/` and `gui/controllers/` sit between widgets and services.

```
widgets → Controllers → services → repositories → CSV
```

A controller is a stateless adapter that holds a reference to `AppContext` and
nothing else. It converts domain objects into view structs (`RouteRow`,
`StatementView`, `StudentRow`, `ComplaintRow`, `NoticeRow`, `SeatView`) in which
money is formatted, percentages are clamped, and a status is reduced to a token a
table cell can colour. Every operation returns `ActionResult`, so a page never
interprets a service enum or an out-parameter.

Why this matters beyond tidiness: "outstanding" is computed once, in
`PaymentController`. The dashboard card, the fee summary, the roster column and
the reports financial block all read it, so they cannot drift apart. The
original project had a single `due` field that the total re-added a
registration fee to, which is exactly the class of bug a shared computation
prevents.

## Why the GUI does not own a `SeatRepository`

The domain has no seat repository, and that is deliberate rather than an
omission. Seat ownership lives on the `Student` record (`routeId` +
`seatNumber`), and `SeatService` is the only code permitted to write it. A
separate `seats.csv` would introduce a second source of truth and make
"one seat, one student" impossible to guarantee. Occupancy is therefore always
derived by asking which students hold which seat.

## Dependency direction

- Domain → nothing above it. No Qt, no console, no knowledge of either front end.
- Controllers → domain, plus Qt's `QString` for view structs.
- Widgets → controllers. A page never includes a service header.

`AppContext` is the one place that knows all the services exist, which is what
keeps construction explicit and avoids a service locator sprinkled through the
pages.

## Data directory

Both front ends read and write the same CSV files. The GUI resolves the folder
from `ST_DATA_DIR`, then `./data`, then `../data`, because a desktop binary can
be launched from the repository root, from `build/bin`, or from an IDE. The
console app resolves against the working directory. Accounts and payments are
therefore interchangeable between the two, and the one-shot legacy import cannot
run twice.
