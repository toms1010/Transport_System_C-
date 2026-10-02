#include "TestHarness.hpp"

#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "services/PaymentService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "utils/Validation.hpp"

#include <filesystem>

using namespace st;

namespace {

struct PaymentFixture {
    std::filesystem::path dir;
    CsvUserRepository users;
    CsvRouteRepository routesRepo;
    CsvPaymentRepository paymentsRepo;
    ConfigService config;
    AuthService auth;
    RouteService routes;
    SeatService seats;
    PaymentService payments;

    PaymentFixture()
        : dir(std::filesystem::temp_directory_path() / "st_test_payments"),
          users((dir / "users.csv").string()),
          routesRepo((dir / "routes.csv").string()),
          paymentsRepo((dir / "payments.csv").string()),
          config(),
          auth(users, paymentsRepo, config),
          routes(auth, routesRepo),
          seats(routes, auth, users),
          payments(paymentsRepo, users, auth, config) {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        config.load((dir / "config.csv").string());
        auth.loadUsers();
        auth.loadJournal();
        routes.load();
        payments.load();
    }

    ~PaymentFixture() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    // The transport fee is set when a seat is allotted, so these tests take a
    // real seat on R1 (fare 9000) to reach a realistic 2500 + 9000 = 11500 state.
    int nextSeat = 1;

    std::string addStudent(const std::string& username, long long fare) {
        RegistrationForm f;
        f.fullName = username;
        f.fatherName = "Parent";
        f.phone = "9876543210";
        f.address = "Campus";
        f.username = username;
        f.password = "secret123";
        auth.registerStudent(f);
        std::string userId;
        for (const auto& user : auth.userCache()) {
            if (user->username() == username) userId = user->userId();
        }
        if (userId.empty()) return "";

        // The transport fee is set when a seat is allotted, so these tests take a
        // real seat. Pinning a route by fare lets a test use a specific amount.
        std::string target = "R1";
        for (const Route& r : routes.cache()) {
            if (r.fare() == fare) target = r.routeId();
        }
        std::string detail;
        seats.assign(userId, target, nextSeat++, detail);
        return userId;
    }

    Student* student(const std::string& userId) { return auth.findStudent(userId); }
};

}

ST_TEST(totalAmountCombinesBothBuckets) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);
    const Student* s = fx.student(id);
    EXPECT_EQ(s->registrationFee(), 2500LL);
    EXPECT_EQ(s->transportFee(), 9000LL);
    EXPECT_EQ(s->totalAmount(), 11500LL);
    EXPECT_EQ(s->outstanding(), 11500LL);
}

ST_TEST(partialPaymentLowersOutstanding) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);

    const PaymentReceipt receipt = fx.payments.recordPayment(id, FeeKind::Transport, 5000, "cashier");
    EXPECT_TRUE(receipt.ok);
    EXPECT_EQ(receipt.appliedAmount, 5000LL);
    EXPECT_EQ(fx.student(id)->totalPaid(), 5000LL);
    EXPECT_EQ(fx.student(id)->outstanding(), 6500LL);
}

ST_TEST(overpaymentIsCappedAtOutstanding) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);

    const PaymentReceipt receipt = fx.payments.recordPayment(id, FeeKind::Transport, 999999, "cashier");
    EXPECT_TRUE(receipt.ok);
    EXPECT_EQ(receipt.appliedAmount, 9000LL);
    EXPECT_EQ(fx.student(id)->transportPaid(), 9000LL);
    EXPECT_EQ(fx.student(id)->outstanding(), 2500LL);

    const PaymentReceipt second = fx.payments.recordPayment(id, FeeKind::Registration, 999999, "cashier");
    EXPECT_TRUE(second.ok);
    EXPECT_EQ(second.appliedAmount, 2500LL);
    EXPECT_EQ(fx.student(id)->outstanding(), 0LL);
}

ST_TEST(registrationAndTransportSettleIndependently) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);

    // The original design could never clear the full 11,500 because payment was
    // always capped against a single `due` field while total included a fixed
    // registration fee. Two buckets fixes it.
    fx.payments.recordPayment(id, FeeKind::Transport, 9000, "cashier");
    EXPECT_EQ(fx.student(id)->outstanding(), 2500LL);

    fx.payments.recordPayment(id, FeeKind::Registration, 2500, "cashier");
    EXPECT_EQ(fx.student(id)->outstanding(), 0LL);
    EXPECT_TRUE(fx.student(id)->fullyPaid());
}

ST_TEST(fullPaymentSettlesAccount) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);
    fx.payments.recordPayment(id, FeeKind::Transport, 9000, "cashier");
    fx.payments.recordPayment(id, FeeKind::Registration, 2500, "cashier");

    EXPECT_TRUE(fx.student(id)->fullyPaid());
    EXPECT_EQ(fx.payments.statementFor(id).statusLabel(), std::string("PAID"));
}

ST_TEST(negativeOrZeroPaymentIsRejected) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);
    EXPECT_TRUE(fx.payments.recordPayment(id, FeeKind::Transport, 0, "cashier").ok == false);
    EXPECT_TRUE(fx.payments.recordPayment(id, FeeKind::Transport, -500, "cashier").ok == false);
    EXPECT_EQ(fx.student(id)->totalPaid(), 0LL);
}

ST_TEST(payingSettledAccountIsRejected) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);
    fx.payments.recordPayment(id, FeeKind::Transport, 9000, "cashier");
    fx.payments.recordPayment(id, FeeKind::Registration, 2500, "cashier");

    const PaymentReceipt receipt = fx.payments.recordPayment(id, FeeKind::Transport, 100, "cashier");
    EXPECT_TRUE(receipt.ok == false);
    EXPECT_TRUE(receipt.message.find("no outstanding") != std::string::npos);
}

ST_TEST(settledBucketRejectsFurtherPayment) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);
    fx.payments.recordPayment(id, FeeKind::Registration, 2500, "cashier");

    const PaymentReceipt receipt = fx.payments.recordPayment(id, FeeKind::Registration, 500, "cashier");
    EXPECT_TRUE(receipt.ok == false);
    EXPECT_TRUE(receipt.message.find("already fully paid") != std::string::npos);
}

ST_TEST(paymentForUnknownStudentFails) {
    PaymentFixture fx;
    const PaymentReceipt receipt = fx.payments.recordPayment("STU9999", FeeKind::Transport, 100, "x");
    EXPECT_TRUE(receipt.ok == false);
    EXPECT_TRUE(receipt.message.find("not found") != std::string::npos);
}

ST_TEST(paymentHistoryIsJournalled) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);
    fx.payments.recordPayment(id, FeeKind::Transport, 4000, "cashier");
    fx.payments.recordPayment(id, FeeKind::Transport, 3000, "cashier");

    const std::vector<Payment> history = fx.payments.historyFor(id);
    EXPECT_EQ(history.size(), static_cast<std::size_t>(2));
    EXPECT_EQ(history[0].amount(), 4000LL);
    EXPECT_EQ(history[1].amount(), 3000LL);
    EXPECT_TRUE(history[0].reference().rfind("RCPT", 0) == 0);
}

ST_TEST(paymentPersistsAcrossReload) {
    PaymentFixture fx;
    const std::string id = fx.addStudent("asha", 9000);
    fx.payments.recordPayment(id, FeeKind::Transport, 5000, "cashier");

    PaymentService reloaded(fx.paymentsRepo, fx.users, fx.auth, fx.config);
    reloaded.load();
    const FeeBreakdown statement = reloaded.statementFor(id);
    EXPECT_EQ(statement.transportPaid, 5000LL);
    EXPECT_EQ(statement.outstanding(), 6500LL);
}

ST_TEST(aggregateTotalsAreCorrect) {
    PaymentFixture fx;
    const std::string a = fx.addStudent("asha", 9000);
    const std::string b = fx.addStudent("bilal", 8000);
    fx.payments.recordPayment(a, FeeKind::Transport, 9000, "cashier");
    fx.payments.recordPayment(b, FeeKind::Transport, 4000, "cashier");

    EXPECT_EQ(fx.payments.collectedTotal(), 13000LL);
    EXPECT_EQ(fx.payments.studentsWithDues(), 2);
    EXPECT_EQ(fx.payments.outstandingTotal(), 2500LL + 6500LL);
}

int main() {
    std::cout << "test_payments\n";
    return ::st::testing::Registry::instance().run() == 0 ? 0 : 1;
}