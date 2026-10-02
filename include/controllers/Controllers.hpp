#pragma once

#include "controllers/AuthController.hpp"
#include "controllers/ComplaintController.hpp"
#include "controllers/NoticeController.hpp"
#include "controllers/PaymentController.hpp"
#include "controllers/RouteController.hpp"
#include "controllers/SeatController.hpp"
#include "controllers/StaffController.hpp"
#include "controllers/StudentController.hpp"

namespace gui {

class AppContext;

// Single owner of every controller, constructed once per window.
//
// Pages never touch AppContext's services directly: they are handed this bundle
// and read view-ready rows from it, which keeps button handlers free of
// business logic and keeps the domain layer free of Qt.
class Controllers {
public:
    explicit Controllers(AppContext& context);

    AuthController& auth() { return auth_; }
    RouteController& routes() { return routes_; }
    SeatController& seats() { return seats_; }
    PaymentController& payments() { return payments_; }
    ComplaintController& complaints() { return complaints_; }
    NoticeController& notices() { return notices_; }
    StudentController& students() { return students_; }
    StaffController& staff() { return staff_; }

    AppContext& context() { return context_; }

private:
    AppContext& context_;
    AuthController auth_;
    RouteController routes_;
    SeatController seats_;
    PaymentController payments_;
    ComplaintController complaints_;
    NoticeController notices_;
    StudentController students_;
    StaffController staff_;
};

}  // namespace gui