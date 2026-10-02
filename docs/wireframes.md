# Wireframes

Rendering notes: the application clamps itself to the terminal width (60-98
columns). Menus highlight with `▶` and redraw in place. Status tags are
`[OK] [INFO] [WARNING] [ERROR] [OPEN] [RESOLVED] [PAID] [PENDING]`.

---

## 1. Main menu

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                            CBIT TRANSPORT MANAGEMENT
                        CBIT  •  Transport Cell, Hyderabad
  ╚══════════════════════════════════════════════════════════════════════════════╝

  [INFO] Register, sign in, browse routes and read notices.
  Main Menu
  ──────────────────────────────────────────────────────────────────────────────
  ↑/↓ navigate    ENTER select    1-9 quick select    ESC back
  ▶ Login
    Register
    View Routes
    Notice Board
    Help / Contact
    Exit
                                    (blank rows)
  StudentTransport CBIT  ·  itsourcecode.com
```

On narrow terminals the box, table columns and key hint shrink to fit, and long
values are trimmed with a trailing `~` rather than wrapping:

```text
  ID   PATH                       VIA                 FARE       SEATS   FREE
  ──────────────────────────────────────────────────────────────────────────────
  R1   Ameerpet -> Uppal          Kachiguda, Secunder~Rs 9,000   12/30   18
```

## 2. Login

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                                    LOGIN
                               Sign in as STUDENT
  ╚══════════════════════════════════════════════════════════════════════════════╝

  Username                asha
  Password                ********

  [ERROR] No account matches that username.
```

On success:

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                               STUDENT DASHBOARD
                                  Asha Rao
  ╚══════════════════════════════════════════════════════════════════════════════╝
```

## 3. Registration

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                                 REGISTRATION
                               Student account
  ╚══════════════════════════════════════════════════════════════════════════════╝

  [INFO] Fields marked * are required.

  Full name *               Asha Rao
  Father / Guardian         Kiran Rao
  Mobile number *           9876543210
  Address *                 Hostel Block C
  Username [STU001]         asha
  Password *                ********
  Confirm password *        ********
  [INFO] Password strength: fair. Store it safely.
  Create this account? [Y/n]

  ╔══════════════════════════════════════════════════════════════════════════════╗
                                CONFIRM
  ╚══════════════════════════════════════════════════════════════════════════════╝

  Username                STU001
  Role                    STUDENT
  Name                    Asha Rao
  Guardian                Kiran Rao
  Mobile                  9876543210
  Address                 Hostel Block C
  Route                   R2  Gachibowli -> Secunderabad
  Seat number             6
  Payable now             Rs 11,000
```

Staff adds one step:

```text
  [WARNING] Staff accounts require the authorization code issued by the
            transport office.
  Authorization code       ********
```

## 4. Student dashboard

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                              STUDENT DASHBOARD
                                 Asha Rao
  ╚══════════════════════════════════════════════════════════════════════════════╝

  Student Menu
  ──────────────────────────────────────────────────────────────────────────────
  Student ID              STU0001
  Route                   R1  Ameerpet -> Uppal
  Seat                    12
  Payment                 [PARTIALLY PAID]
  Outstanding             Rs 6,500
  ↑/↓ navigate    ENTER select    1-9 quick select    ESC back
  ──────────────────────────────────────────────────────────────────────────────
  ↑/↓ navigate    ENTER select    1-9 quick select    ESC back
  ▶ My Transport
    Route & Seat
    Payments & Dues
    Allotment Card
    Complaints
    Notice Board
    Profile
    Change Password
    Help / Contact
    Logout
```

## 5. Route selection

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                                 ROUTE & SEAT
                                 Asha Rao
  ╚══════════════════════════════════════════════════════════════════════════════╝

  Current route            R1  Ameerpet -> Uppal
  Current seat             12
  Choose a route
  ──────────────────────────────────────────────────────────────────────────────
  ▶ R1  Ameerpet -> Uppal               Rs 9,000  12/30
    R2  Gachibowli -> Secunderabad      Rs 8,500  30/30
    R3  Miyapur -> L.B.Nagar            Rs 9,500  10/25
    R4  Kukatpally -> Charminar         Rs 8,000  17/25
    R5  Begumpet -> Shamshabad          Rs 7,000  15/20
```

Public route list:

```text
                                 ROUTES & FARES
  ──────────────────────────────────────────────────────────────────────────────
  ID   PATH                       VIA                 FARE       SEATS   FREE
  ──────────────────────────────────────────────────────────────────────────────
  R1   Ameerpet -> Uppal          Kachiguda, Secunder~Rs 9,000   12/30   18
  R2   Gachibowli -> Secunderabad Nanakramguda, Banja~Rs 8,500   30/30   0
  R3   Miyapur -> L.B.Nagar       Kukatpally, Moosara~Rs 9,500   10/25   15
  R4   Kukatpally -> Charminar    Erragadda, Ameerpet Rs 8,000   17/25   8
  R5   Begumpet -> Shamshabad     Erragadda, Gachibow~Rs 7,000   15/20   5
  ──────────────────────────────────────────────────────────────────────────────
  [INFO] Registration fee of Rs 2,500 applies to every new student.
```

## 6. Seat selection

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                                SELECT A SEAT
                            R1  Ameerpet -> Uppal
  ╚══════════════════════════════════════════════════════════════════════════════╝

  Seat Map  R1  Ameerpet -> Uppal
  ──────────────────────────────────────────────────────────────────────────────

  Ameerpet -> Uppal
  Via         Kachiguda, Secunderabad
  Fare        Rs 9,000
  Occupancy   1/30   (29 seats available)

  FRONT OF BUS

  [01]  [02]  [03]✓  [04]  [05]  [06]
  [07]  [X ]  [09]  [10]  [11]  [12]
  [13]  [14]  [15]  [16]  [17]  [18]
  [19]  [20]  [21]  [22]  [23]  [24]
  [25]  [26]  [27]  [28]  [29]  [30]

  [ n ] AVAILABLE   [X ] OCCUPIED   [n]✓ SELECTED
```

## 7. Payments and dues

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                               PAYMENTS & DUES
                                 Asha Rao
  ╚══════════════════════════════════════════════════════════════════════════════╝

  Total amount            Rs 11,500
  Total paid              Rs 5,000
  Outstanding             Rs 6,500
  Status                  PARTIALLY_PAID

  Payment History
  REF       DATE              TYPE          AMOUNT      REFERENCE     BY
  ──────────────────────────────────────────────────────────────────────────────
  PAY00001  2026-10-02 00:07  REGISTRATION  Rs 2,500    RCPT3D564FE5 STU001
  PAY00002  2026-10-02 00:07  TRANSPORT     Rs 2,500    RCPT91AB77C2 STU001
  ──────────────────────────────────────────────────────────────────────────────

  Make a payment
  ──────────────────────────────────────────────────────────────────────────────
  ▶ Pay registration fee (Rs 2,500)
    Pay transport fee (Rs 9,000)
    Back

  Amount to pay          2000

  Receipt
  ──────────────────────────────────────────────────────────────────────────────
  Payment ID              PAY00003
  Reference               RCPT7F21AA90CD
  Bucket                  TRANSPORT
  Amount applied          Rs 2,000
  Outstanding after       Rs 4,500
```

Statement view (used by the staff-side report):

```text
  Payment & Dues  Asha Rao
  ──────────────────────────────────────────────────────────────────────────────
  Student ID              STU0001
  Username                asha
  Registration fee        Rs 2,500   [PAID]
  Transport fee           Rs 9,000   [PENDING]
  ──────────────────────────────────────────────────────────────────────────────
  Total amount            Rs 11,500
  Total paid              Rs 2,500
  Outstanding             Rs 9,000
```

## 8. Allotment card

```text
                              ALLOTMENT CARD
                                 Asha Rao
  ──────────────────────────────────────────────────────────────────────────────
  Student                   Asha Rao
  Student ID                STU0001
  Guardian                  Kiran Rao
  Route                     R1  Ameerpet -> Uppal
  Seat number               12
  Annual fare               Rs 9,000
  ──────────────────────────────────────────────────────────────────────────────
  Total amount              Rs 11,500
  Outstanding               Rs 9,000
  Payment status            PENDING
  Signature                 __________________________
```

## 9. Complaints

Student view:

```text
                              MY COMPLAINTS
                                 Asha Rao

  CMP0001  Bus arrives late every morning
    2026-10-02 00:12   [OPEN]

  CMP0000  Seat number not printed on card
    2026-10-01 09:30   [RESOLVED]
    Response: Seat card reprinted at the transport office.  (STF0001, ...)

  ┌──────────────────────────────────────────┐
  │ Submit a complaint                       │
  │ View full details                        │
  │ Back                                     │
  └──────────────────────────────────────────┘
```

Staff view:

```text
                               STAFF DASHBOARD
                              Transport Staff
  ╚══════════════════════════════════════════════════════════════════════════════╝

  Staff ID                STF0001
  Assigned route          R1  Ameerpet -> Uppal

  TRANSPORT MANAGEMENT OVERVIEW

  Students on my route    12
  Active routes           5
  Open complaints         1
  Students with dues      2
  Total outstanding       Rs 47,500
  Staff Menu
  ──────────────────────────────────────────────────────────────────────────────
  ▶ My Route Assignment
    Student Roster
    Route Management
    Seat Allocation
    Payments & Dues
    Complaints
    Notice Board
    Reports
    Help / Contact
    Logout
```

My Route Assignment:

```text
                              MY ROUTE ASSIGNMENT
                                Kiran Rao

  Current assignment            none yet
  Choose the route you are responsible for
  ──────────────────────────────────────────────────────────────────────────────
  ▶ R1  Ameerpet -> Uppal   1/30
    R2  Gachibowli -> Secunderabad   0/30
    R3  Miyapur -> L.B.Nagar   0/25
    R4  Kukatpally -> Charminar   0/25
    R5  Begumpet -> Shamshabad   0/20
    Clear my assignment

  Take responsibility for route R1? [Y/n] y
  [OK] Assignment updated.
```

Student roster:

```text
                               STUDENT ROSTER
  ──────────────────────────────────────────────────────────────────────────────
  ID       USERNAME  NAME                  PHONE        ROUTE  SEAT  OUTSTANDING  STATUS
  ──────────────────────────────────────────────────────────────────────────────
  STU0001  asha      Asha Rao              9876543210  R1     12    Rs 9,000     PENDING
  STU0002  bilal     Bilal Khan            9812345678  R1     7     Rs 0         PAID
  ──────────────────────────────────────────────────────────────────────────────
  Students shown          2
  Outstanding             Rs 9,000
```

## 10. Admin dashboard

```text
                               ADMIN DASHBOARD
                           Transport Administrator
  ╚══════════════════════════════════════════════════════════════════════════════╝

  Students                124
  Staff accounts          3
  Routes                  5
  Open complaints         12
  Outstanding             Rs 1,240,000
  Collected               Rs 2,760,000
  Admin Menu
  ──────────────────────────────────────────────────────────────────────────────
  ▶ Student Roster
    Staff Roster
    Route Management
    Seat Allocation
    Payments & Dues
    Complaints
    Notice Board
    Reports
    Help / Contact
    Logout
```

## 11. Notices

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                               NOTICE BOARD
                              Transport Staff
  ╚══════════════════════════════════════════════════════════════════════════════╝

  ID     TITLE                                  AUTHOR      CREATED
  ──────────────────────────────────────────────────────────────────────────────
  NOT001 Holiday Schedule                        STF0001     2026-10-01 08:30
  ──────────────────────────────────────────────────────────────────────────────
  Notice actions
  ──────────────────────────────────────────────────────────────────────────────
  ▶ Publish a notice
    Read a notice
    Edit a notice
    Delete a notice
    Back

  NOT001  Holiday Schedule
  ──────────────────────────────────────────────────────────────────────────────
  Author                 STF0001
  Created                2026-10-01 08:30

  Content
    Buses run on a holiday timetable from 01 October.
```

## 12. Reports

```text
                                 REPORTS
                              Transport Staff

  ROUTE  PATH                       STUDENTS  CAPACITY  FREE    BILLED VALUE
  ──────────────────────────────────────────────────────────────────────────────
  R1     Ameerpet -> Uppal          12        30        18      Rs 108,000
  R2     Gachibowli -> Secunderabad 30        30        0       Rs 255,000
  ──────────────────────────────────────────────────────────────────────────────
  Registered students      124
  Active routes            5
  Open complaints          12
  Total collected          Rs 2,760,000
  Total outstanding        Rs 1,240,000
  Notices published        8
```

## 13. First run

```text
  ╔══════════════════════════════════════════════════════════════════════════════╗
                               FIRST RUN SETUP
                        CBIT  •  Transport Cell, Hyderabad
  ╚══════════════════════════════════════════════════════════════════════════════╝

  [OK] An administrator account was created for you.
  Username                admin
  Password                admin123
  [WARNING] Change this password immediately after signing in.
```

---

## Desktop (Qt Widgets) wireframes

The screens below are the Qt front end. They carry the same features as the
console flow above; the differences are the things a mouse-driven interface can
do that a terminal cannot: simultaneous panels, inline filtering, progress bars,
toasts, printing and a second theme.

## Window shell

```text
┌───────────────┬────────────────────────────────────────────────────────┐
│ CBIT          │ Dashboard                              ☾       TA       │
│ TRANSPORT     │                          Transport Administrator     │
│               │                          ADMIN · @admin              │
│ ▦ Dashboard   ├────────────────────────────────────────────────────────┤
│ 🚌 My transport│  Welcome back, Transport Administrator              │
│ 💳 Payments   │  Transport Cell · 09:00 - 16:00, Monday to Saturday   │
│ ✉ Complaints │                                                        │
│ ◈ Notices     │  ┌────────────┐ ┌────────────┐ ┌────────────┐         │
│ ▣ Allotment   │  │ 🚌 ROUTE   │ │ 💺 SEAT    │ │ 💳 PAYMENT │         │
│ ◍ Profile     │  │ R1         │ │ 9          │ │ ₹9,000     │         │
│               │  │ Ameerpet → │ │ Ameerpet → │ │ DUES       │         │
│ ? Help        │  │ Uppal      │ │ Uppal      │ │ PENDING    │         │
│ ⚙ Settings    │  └────────────┘ └────────────┘ └────────────┘         │
│               │  ┌───────────────────────┐ ┌───────────────────────┐  │
│               │  │ 📢 Latest notices     │ │ 📝 Recent complaints │  │
│               │  │ Route 4 timing change │ │ Bus Delay   RESOLVED │  │
│               │  │ 02 October 2026       │ │ Seat problem   OPEN  │  │
│ ⇥ Sign out    │  └───────────────────────┘ └───────────────────────┘  │
├───────────────┴────────────────────────────────────────────────────────┤
│ Data folder: ~/Transport_System_C++/data                             │
└────────────────────────────────────────────────────────────────────────┘
```

Sidebar width is fixed at 244px; everything to its right is a layout that
reflows. Minimum window 1120×680.

### Student — seat map

```text
┌────────────────────────────────────────────────────────────────────────┐
│ My transport                                                           │
│ Pick a route, then click a free seat and claim it.                     │
│                                                                        │
│ Route   [ R1 · Ameerpet - Uppal — Ameerpet → Uppal ▼ ]  Change route  │
├───────────────────────────────────────────────┬────────────────────────┤
│ Seat layout                                   │ My seat                │
│ 30 seats · 3 occupied · 27 free · fare 9,000  │ Holds seat 9 on       │
│                                               │ Ameerpet - Uppal.      │
│   F R O N T   /   D R I V E R                │ Ananya Sharma          │
│                                               │ @ananya                │
│   ┌────┐ ┌────┐ ┌────┐ ┌────┐                │ [ Release my seat ]    │
│   │ 01 │ │ 02 │ │ 03 │ │ 04 │                ├────────────────────────┤
│   └────┘ └────┘ └────┘ └────┘                │ How this works         │
│   ┌────┐ ┌────┐ ┌────┐ ┌────┐                │ Your transport fee     │
│   │ 05 │ │ 06 │ │ 07 │ │ 08 │                │ starts once you take   │
│   └────┘ └────┘ └────┘ └────┘                │ a seat.                │
│   ┌────┐ ┌────┐ ┌────┐ ┌────┐                └────────────────────────┤
│   │ 09 │ │ 10 │ │ 11 │ │ 12 │   ← 09 is green: yours               │
│   └────┘ └────┘ └────┘ └────┘                                       │
│                                                                       │
│  ▢ Free  ▣ Occupied  ▢ Your seat  ▮ Selected      [ Confirm seat ]    │
└───────────────────────────────────────────────┴────────────────────────┘
```

Occupied seats are disabled and greyed. Clicking a free seat turns it indigo;
the confirm button appears only while a free seat is selected.

### Staff — seat allocation

Same grid, plus a student selector on the right and three actions. Staff see
whose seat they are about to change before they change it.

```text
│ Seat map                                                             │
│ Route [ R1 ▼ ]            Student [ Ananya Sharma (@ananya) · seat 9 ▼]│
│ ┌────────────────────────────────────┬──────────────────────────────┐ │
│ │ Seat layout                        │ Selected student              │ │
│ │  [01][02][03][04]                   │ Ananya Sharma holds seat 9    │ │
│ │  [05][06][07][08]                   │ on Ameerpet - Uppal.          │ │
│ │  [09][10][11][12]                   │ [ Assign the selected seat ]  │ │
│ │                                    │ [ Auto-assign first free ]    │ │
│ │ ▢ Free ▣ Occupied ▢ Yours ▮ Sel.   │ [ Release this seat ]         │ │
│ └────────────────────────────────────┴──────────────────────────────┘ │
```

### Payments — student view

```text
┌────────────────────────────────────────────────────────────────────────┐
│ Payments & dues                                                        │
│ Registration and transport fees are separate balances, so a payment      │
│ always lands in the bucket you choose.                                  │
├───────────────────────────────────────────────┬────────────────────────┤
│ Fee summary                                   │ Make a payment         │
│ Ananya Sharma                                │ Payment type           │
│ @ananya · Ameerpet - Uppal · Seat 9           │ [ Transport fee      ▼]│
│                                               │ Amount      [ ₹ 9000 ] │
│ DUES PENDING · total 11,500                   │ Reference   [ UPI 4721 ]│
│ 9,000                                         │ [ Confirm payment   ]  │
│                                               │ [ Settle this fee in    │
│ REGISTRATION   2,500 of 2,500          0       │   full ]               │
│ ████████████████████████████████              │                        │
│ TRANSPORT   0 of 9,000              9,000      │                        │
│ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░              │                        │
├───────────────────────────────────────────────┴────────────────────────┤
│ Payment history                                        [ Export CSV ]  │
│ Receipt        Date          Fee type      Recorded by        Amount   │
│ RCPT538EDD…    2026-10-02    REGISTRATION  Transport Admin     2,500   │
└────────────────────────────────────────────────────────────────────────┘
```

Each fee bucket has its own progress bar, because registration and transport are
settled independently.

### Reports

```text
┌────────────────────────────────────────────────────────────────────────┐
│ Reports                                    [ ⬇ Export payment ledger ] │
│ ┌────────────────────────────────────────────────────────────────────┐ │
│ │ Financial summary                                                   │ │
│ │ Expected        45,500                                             │ │
│ │ Collected        3,700                                             │ │
│ │ Outstanding     41,800                                             │ │
│ │ Students        4 total, 4 with dues                              │ │
│ └────────────────────────────────────────────────────────────────────┘ │
│ ┌────────────────────────────────────────────────────────────────────┐ │
│ │ Route occupancy                        4 of 130 seats across 5 routes│ │
│ │ Route            Fare    Occupied   Capacity   Utilisation          │ │
│ │ R1 · Ameerpet–Up 9,000   3          30         ███░░░░░░░░░  10%   │ │
│ │ R2 · Gachibowli–S 8,500   1          30         █░░░░░░░░░░░░   3%  │ │
│ └────────────────────────────────────────────────────────────────────┘ │
│ ┌────────────────────────────┐ ┌────────────────────────────────────┐  │
│ │ Students by route          │ │ Complaints                          │  │
│ │ Route        Students  Due │ │ Total      2                        │  │
│ │ Ameerpet–Up      3  30,800 │ │ Open       2                        │  │
│ │ Gachibowli–S     1  11,000 │ │ Resolved   0                        │  │
│ └────────────────────────────┘ └────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────┘
```

### Registration wizard

```text
┌────────────────────────────────────────────────────────────────────────┐
│ Create a student account                                               │
│ Pick a route and a seat as part of signing up.                         │
│                                                                        │
│ 1. Personal  ›  2. Account  ›  3. Transport  ›  4. Summary             │
├────────────────────────────────────────────────────────────────────────┤
│  Full name             [ Ananya Sharma                              ]  │
│  Father / guardian     [ Ravi Sharma                                ]  │
│  Phone                 [ 9876543210                                  ]  │
│  Address               [ Hostel Block C, Room 214                   ]  │
│                                                                        │
│                                              [ Back ]      [ Next ]    │
└────────────────────────────────────────────────────────────────────────┘
```

Step 3 opens a route picker (cards showing fare, capacity and free seats) and
then a seat grid. The account is written on the final step only, so abandoning
the wizard leaves no half-created account behind.

### Login

```text
┌──────────────────────────────────────────────┐
│  🚌                                        │
│  Student Transport                         │
│  Management System                         │
│  CBIT • Transport Cell, Hyderabad           │
├──────────────────────────────────────────────┤
│  Sign in to continue                        │
│  [ Username                            ]    │
│  [ Password                            ]    │
│  ☐ Show password                            │
│  [ Sign in ]                                │
│  Forgot password?                           │
│  Create a student account  Register as staff│
│  View routes   Notice board   About         │
│  Trouble signing in? Contact …              │
└──────────────────────────────────────────────┘
```

The three links at the bottom open the public window — routes, notices and about
are readable without an account.

### Settings, dark mode, notifications

Settings holds the theme switch, the data folder, and backup and restore.
Restore refuses a partial backup rather than half-replacing the dataset.

Toasts appear bottom-right and fade after about three seconds. Modals are
reserved for destructive actions, and every one defaults to Cancel:

```text
        ┌──────────────────────────────────────┐
        │ ⚠ Delete route                      │
        ├──────────────────────────────────────┤
        │ R3 still has 15 student(s) allotted. │
        │ Reassign them before deleting it.    │
        │                                      │
        │   [ Cancel ]      [ Delete ]         │
        └──────────────────────────────────────┘
```

Dark mode swaps the palette behind the same stylesheet, so the seat grid, the
progress bars, the tables and the inputs all retheme together.
