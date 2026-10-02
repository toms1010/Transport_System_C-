# GUI design

How the Qt Widgets front end is put together, and the rules that keep it
maintainable. The console front end is documented separately; this file covers
the desktop application only.

## Layering

```text
┌───────────────────────────────────────────────────────────────────┐
│ GUI LAYER      widgets, dialogs, pages                           │
│                DashboardPage · SeatMapPage · PaymentsPage · …     │
├───────────────────────────────────────────────────────────────────┤
│ CONTROLLER     AuthController · RouteController · SeatController  │
│ LAYER          PaymentController · ComplaintController ·          │
│                NoticeController · StudentController ·             │
│                StaffController                                   │
├───────────────────────────────────────────────────────────────────┤
│ SERVICE LAYER  AuthService · RouteService · SeatService · …       │
├───────────────────────────────────────────────────────────────────┤
│ REPOSITORY     CsvUserRepository · CsvRouteRepository · …         │
├───────────────────────────────────────────────────────────────────┤
│ DATA LAYER     data/*.csv                                         │
└───────────────────────────────────────────────────────────────────┘
```

The controller layer exists so that no widget ever calls a service directly.
A page slot reads like a description of a user gesture, not a procedure:

```text
void PaymentsPage::settleBucket() {
    if (!confirm(QStringLiteral("Settle in full"), ...)) return;
    ReceiptView receipt;
    const ActionResult result = controllers_.payments().settleBucket(...);
    toast(result);
    updateStatement();
    updateHistory();
}
```

Three consequences worth keeping:

- **All rules stay in one place.** "Outstanding" is computed in
  `PaymentController::statement`, so the dashboard, the payments page and the
  reports page cannot disagree.

- **Controllers are stateless.** Each one holds a reference to `AppContext` and
  nothing else, so they are cheap to construct and safe to share.

- **Services stay Qt-free.** The console front end and the 69 tests keep
  compiling against exactly the same code as before the GUI existed.

### View structs

Controllers return display-ready rows (`RouteRow`, `StatementView`,
`StudentRow`, `ComplaintRow`, `NoticeRow`, `SeatView`) rather than model
pointers. Money is already formatted, percentages already clamped, statuses
already reduced to a `kind` string a table cell can colour. Pages therefore
contain layout and event wiring only.

## Navigation

`MainWindow` owns a sidebar of checkable buttons in a `QButtonGroup` and a
`QStackedWidget` of pages. The label list is derived from the session role:

| Role | Navigation |
| ------ | ------------ |
| Student | Dashboard, My transport, Payments & dues, My complaints, Notice board, Allotment card, My profile, Help & contact, Settings |
| Staff | Dashboard, Students, Routes, Seat map, Payments & dues, Complaints, Notices, Reports, My profile, Help & contact, Settings |
| Admin | as Staff, plus Staff accounts |

One window serves all three roles rather than three separate windows. The role
changes which nav entries exist and which page variants render (a student has
no student picker on the seat map; an admin sees the staff list).

Two details that are easy to get wrong:

- `QButtonGroup::buttonClicked` is emitted **by the group**, so `sender()` inside
  the slot is the group and not the button. The slot takes the button as a
  parameter.

- `setChecked()` does not emit `buttonClicked`, so the opening page is refreshed
  explicitly after construction.

Pages can hand control to a sibling without knowing about `MainWindow`:

```text
if (auto* page = siblingPage<SeatMapPage>()) {
    page->selectStudent(id);
    activate(page);
}
```

## Styling

There is exactly one stylesheet, `resources/styles/app.qss`. Every colour in it
is a `{{TOKEN}}`; `Theme.cpp` substitutes the active palette. No widget file
hardcodes a colour, and light and dark share the same rules.

`Palette` carries the token values for one theme. `color::success()` and friends
resolve against `activePalette()`, so widget code can ask for a semantic colour
without threading a palette through every call.

Two themes are defined — `lightPalette()` and `darkPalette()`. The choice is
persisted in `QSettings` under `appearance/theme` and can be changed from the
top bar or from Settings. Switching re-substitutes and re-applies the sheet.

`resources/icons/` holds 31 inline SVGs drawn with `currentColor`, registered
through `resources.qrc`. Nothing is fetched at runtime, so the application has no
network dependency and works offline.

## Notifications

Two channels, deliberately separated:

- **Toasts** (`Notify.hpp`) for routine outcomes — a payment recorded, a seat
  assigned, a notice published. Auto-dismiss after ~3.2s with a fade, stacked
  bottom-right, colour-coded by `kind`.

- **Modal dialogs** for anything destructive or critical: deleting a route,
  restoring a backup, signing out. Every destructive action goes through
  `Page::confirm`, which styles the buttons and defaults to Cancel.

Long operations are wrapped in `BusyOverlay` so the window is not left looking
frozen.

## Data directory

The console front end resolves `data/` against the current working directory,
which lands in a different place depending on how the binary was launched. The
GUI resolves it explicitly: `ST_DATA_DIR`, then `./data`, then `../data`. Both
front ends read and write the same files, so an account created in one is
immediately visible in the other and the one-shot legacy migration never runs
twice.

## Accessibility

- Tab order follows creation order; the seat grid is a grid of real buttons, so
  arrow keys and Tab both work.

- Every control has a text label. Icons are supplementary, never the only cue.

- Focus is visible: `QPushButton:focus` and the input `:focus` rules both set a
  primary-coloured border.

- Escape closes dialogs; Enter submits login, password change and payment forms.

- Status is never conveyed by colour alone — every status cell also carries a
  word such as `Paid`, `Dues pending` or `Open`.

## Testing approach

The domain layer keeps its 69 tests, which cover every rule the GUI depends on.
The GUI itself is verified by construction: an offscreen harness logs in as each
role, walks every navigation entry, and asserts that the stack switched to the
expected page class, then grabs a screenshot of each screen. That harness caught
three defects that compiled cleanly — the `sender()` bug above, a null
`QStackedWidget` dereference in the wizard constructor, and a `QComboBox`
connect on a pointer that is null for students.

Qt's own layout engine handles every size, so there are no hardcoded pixel
positions. The minimum window is 1120×680 and the layouts are verified from
there up.
