# System Design

## 1. Purpose

A console-based Student Transport Management System for a college transport
cell. It replaces a single-file C++ assignment with a layered C++17
application that manages students, staff, routes, seats, fees, complaints and
notices, and persists all of it to disk.

## 2. Actors

| Actor | Description | Authentication |
|-------|-------------|----------------|
| Student | Enrolled student who needs transport | username + password |
| Staff | Transport office employee | username + password + staff authorization code at registration |
| Administrator | System owner | username + password, bootstrapped on first run |

## 3. Functional requirements

### Authentication
- FR-A1 Register as student with name, guardian, phone, address, username, password.
- FR-A2 Register as staff only with the authorization code from `data/config.csv`
  (compared in constant time).
- FR-A3 Sign in and receive a session carrying user id, username, display name and role.
- FR-A4 Reject empty input, unknown usernames, wrong passwords and deactivated accounts.
- FR-A5 Mask password input and never display or store it in plaintext.
- FR-A6 Change password by verifying the current one first.
- FR-A7 Logout clears the session.

### Routes
- FR-R1 Ship the five specified routes (R1..R5) with their fares and capacities.
- FR-R2 List routes with live occupancy.
- FR-R3 Staff and admin can add, edit and delete routes.
- FR-R4 Capacity may not be reduced below the number of students already allotted.

### Seats
- FR-S1 Show a seat map with `AVAILABLE`, `OCCUPIED` and `SELECTED` states.
- FR-S2 A student can take a seat on a route.
- FR-S3 Two students can never hold the same seat.
- FR-S4 Changing route releases the old seat and sets the transport fee to the new fare.
- FR-S5 A failed route change leaves the student exactly where they were.
- FR-S6 Staff can auto-assign the first free seat to an unassigned student.

### Payments
- FR-P1 Track the registration fee and the transport fee as separate balances.
- FR-P2 `total = registrationFee + transportFee`, `outstanding = total - paid`.
- FR-P3 Record a payment against a chosen bucket, capped at that bucket's balance.
- FR-P4 Keep a payment journal with id, date, amount, type, reference and recorder.
- FR-P5 Reject zero, negative and over-remaining payments.
- FR-P6 Show the student their statement, history and a receipt.
- FR-P7 Show staff per-student and aggregate dues without re-charging settled fees.

### Complaints
- FR-C1 A student files a complaint with a subject (80 chars) and description (2000 chars).
- FR-C2 Complaints start `OPEN`.
- FR-C3 Staff read, resolve with a response, or reopen a complaint.
- FR-C4 Resolution records the responder and timestamp.
- FR-C5 Students see the status and any staff response.

### Notices
- FR-N1 Staff and admin publish (title 80 chars, body 4000 chars), edit and delete notices.
- FR-N2 Anyone can browse the notice board.

### Migration
- FR-M1 Import the pre-refactor `list_of_students` / `login.txt` once.
- FR-M2 Record the import in `data/migration.log`.

## 4. Non-functional requirements

- NFR-1 Builds with CMake on Linux, macOS and Windows using C++17.
- NFR-2 Compiles clean under `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`.
- NFR-3 No platform shell-outs (`system("mkdir ...")`); uses `std::filesystem`.
- NFR-4 Input can never crash the program; every path validates and reports errors.
- NFR-5 No global mutable state; services own their caches.
- NFR-6 Storage is abstracted behind repositories so SQLite can replace CSV.
- NFR-9 All numeric input is parsed strictly; `atoi` is never used on user data.
- NFR-10 Malformed or hand-edited CSV rows are skipped, never trusted.
- NFR-11 Free text fields have explicit maximum lengths.
- NFR-7 Unit tests cover authentication, routes, seats, payments and complaints.
- NFR-8 The UI fits an 80-column terminal and degrades on smaller widths.

## 5. System workflow

```
                    START
                      |
                      v
              +---------------+
              |  Main Menu    |
              +-------+-------+
                      |
        +-------------+-------------+-------------+
        v             v             v             v
    Register        Login       View Routes   Notice Board
        |             |
        v             v
  Validate form   Verify password
        |             |
        |             v
        |        Determine role
        |        +----+--------+
        |        v             v
        |    Student        Staff / Admin
        |    Dashboard      Dashboard
        |        |             |
        |        v             v
        |   Route & Seat   Roster / Routes
        |   Payments       Complaints
        |   Complaints     Notices
        |   Notices        Reports
        |
        +-------- all operations go through
                  Service layer -> Repository layer -> CSV
```

## 6. System context

```
        Student                Staff                 Admin
           |                      |                    |
           +----------------------+--------------------+
                                  |
                        +---------v----------+
                        |    Console UI       |
                        +---------+----------+
                                  |
              +-------------------+-------------------+
              |                                       |
   +----------v-----------+              +------------v-----------+
   |    Service layer     |              |    Service layer       |
   | Auth  Routes  Seats  |              | Reports  Config        |
   | Payments Complaints  |              | Notices  Roster        |
   | Notices              |              +------------+-----------+
   +----------+-----------+                           |
              |                                       |
   +----------v---------------------------------------v-----------+
   |                     Repository layer                          |
   | UserRepository  RouteRepository  PaymentRepository            |
   | ComplaintRepository  NoticeRepository                          |
   +-----------------------------+----------------------------------+
                                 |
                     +-----------v------------+
                     |   data/*.csv (storage)  |
                     +------------------------+
```

## 7. Data design

The CSV schema mirrors a relational design so SQLite can be introduced later
without touching the service layer.

**users.csv** (one row per account)

| Column | Type | Notes |
|--------|------|-------|
| userId | TEXT | `STU0001`, `STF0001`, `ADM0001` |
| username | TEXT | unique, login handle |
| role | TEXT | STUDENT / STAFF / ADMIN |
| fullName | TEXT | |
| fatherName | TEXT | |
| phone | TEXT | 10-15 digits |
| address | TEXT | |
| passwordHash | TEXT | salted digest, never plaintext |
| salt | TEXT | per-user 64-bit salt, hex |
| createdAt | TEXT | `YYYY-MM-DD HH:MM` |
| active | 0/1 | deactivation switch |
| routeId | TEXT | student: current route; staff: blank here |
| seatNumber | INT | student only |
| registrationFee | INT | student only |
| registrationPaid | INT | student only |
| transportFee | INT | student only |
| transportPaid | INT | student only |
| staffRouteId | TEXT | staff: assigned route (set from the staff menu) |

**routes.csv**: routeId, routeName, startLocation, destination, via, fare, capacity, active

**payments.csv**: paymentId, studentId, amount, type, paymentDate, reference, recordedBy

**complaints.csv**: complaintId, studentId, subject, description, createdAt, status, response, resolvedAt, resolvedBy

**notices.csv**: noticeId, title, content, authorId, createdAt

**config.csv**: key|value pairs for registration fee, institution name, contact details and
the staff authorization code

## 8. Payment model

The original code stored a single `due` value plus a global registration fee and
a `paid` boolean, which made full payment impossible: the balance was capped
against `due` while the total added the fee back on every calculation.

The replacement keeps two independent buckets:

```
registrationFee   = 2500   (from data/config.csv)
transportFee      = route.fare()

totalAmount()     = registrationFee + transportFee
outstanding()     = totalAmount() - (registrationPaid + transportPaid)

A payment is applied to ONE bucket and capped at that bucket's remaining balance.
Settling the registration fee never implies the transport fee is paid, and staff
reports read the same two balances the student sees.
```