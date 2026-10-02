#include "TestHarness.hpp"

#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "services/RouteService.hpp"
#include "services/SeatService.hpp"
#include "utils/Validation.hpp"

#include <filesystem>

using namespace st;

namespace {

struct SeatFixture {
    std::filesystem::path dir;
    CsvUserRepository users;
    CsvRouteRepository routesRepo;
    CsvPaymentRepository paymentsRepo;
    ConfigService config;
    RouteService routes;
    AuthService auth;
    SeatService seats;

    SeatFixture()
        : dir(std::filesystem::temp_directory_path() / "st_test_seats"),
          users((dir / "users.csv").string()),
          routesRepo((dir / "routes.csv").string()),
          paymentsRepo((dir / "payments.csv").string()),
          config(),
          routes(auth, routesRepo),
          auth(users, paymentsRepo, config),
          seats(routes, auth, users) {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        config.load((dir / "config.csv").string());
        routes.load();
        auth.userCache() = users.loadAll();
    }

    ~SeatFixture() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    std::string addStudent(const std::string& username, const std::string& secret) {
        RegistrationForm f;
        f.fullName = username;
        f.fatherName = "Parent";
        f.phone = "9876543210";
        f.address = "Campus";
        f.username = username;
        f.password = secret;
        auth.registerStudent(f);
        routes.load();
        for (const auto& user : auth.userCache()) {
            if (user->username() == username) return user->userId();
        }
        return "";
    }

    Student* student(const std::string& userId) { return auth.findStudent(userId); }
};

}

ST_TEST(availableSeatsStartFromCapacity) {
    SeatFixture fx;
    const std::vector<int> free = fx.routes.availableSeats("R1");
    EXPECT_EQ(free.size(), static_cast<std::size_t>(30));
    EXPECT_EQ(free.front(), 1);
}

ST_TEST(assignSeatSetsRouteAndSeat) {
    SeatFixture fx;
    const std::string id = fx.addStudent("asha", "secret123");
    std::string detail;
    EXPECT_TRUE(fx.seats.assign(id, "R1", 12, detail) == SeatResult::Success);

    Student* s = fx.student(id);
    EXPECT_TRUE(s != nullptr);
    EXPECT_EQ(s->routeId(), std::string("R1"));
    EXPECT_EQ(s->seatNumber(), 12);
    EXPECT_EQ(fx.routes.availableSeats("R1").size(), static_cast<std::size_t>(29));
}

ST_TEST(duplicateSeatIsPrevented) {
    SeatFixture fx;
    const std::string first = fx.addStudent("asha", "secret123");
    const std::string second = fx.addStudent("bilal", "secret123");

    std::string detail;
    EXPECT_TRUE(fx.seats.assign(first, "R1", 7, detail) == SeatResult::Success);
    EXPECT_TRUE(fx.seats.assign(second, "R1", 7, detail) == SeatResult::SeatTaken);
    EXPECT_TRUE(detail.find("already allotted") != std::string::npos);

    EXPECT_EQ(fx.routes.occupiedSeats("R1").size(), static_cast<std::size_t>(1));
}

ST_TEST(studentMayKeepTheirOwnSeat) {
    SeatFixture fx;
    const std::string id = fx.addStudent("asha", "secret123");
    std::string detail;
    fx.seats.assign(id, "R1", 9, detail);
    EXPECT_TRUE(fx.seats.assign(id, "R1", 9, detail) == SeatResult::Success);
}

ST_TEST(outOfRangeSeatIsRejected) {
    SeatFixture fx;
    const std::string id = fx.addStudent("asha", "secret123");
    std::string detail;
    EXPECT_TRUE(fx.seats.assign(id, "R1", 0, detail) == SeatResult::InvalidSeat);
    EXPECT_TRUE(fx.seats.assign(id, "R1", 31, detail) == SeatResult::InvalidSeat);
    EXPECT_TRUE(fx.seats.assign(id, "R1", -4, detail) == SeatResult::InvalidSeat);
}

ST_TEST(seatNumberValidationRespectsCapacity) {
    std::string reason;
    EXPECT_TRUE(validate::seatNumber(1, 30, reason));
    EXPECT_TRUE(validate::seatNumber(30, 30, reason));
    EXPECT_TRUE(validate::seatNumber(31, 30, reason) == false);
    EXPECT_TRUE(validate::seatNumber(1, 0, reason) == false);
}

ST_TEST(releaseFreesTheSeat) {
    SeatFixture fx;
    const std::string id = fx.addStudent("asha", "secret123");
    std::string detail;
    fx.seats.assign(id, "R1", 5, detail);
    EXPECT_EQ(fx.routes.availableSeats("R1").size(), static_cast<std::size_t>(29));

    EXPECT_TRUE(fx.seats.release(id, detail) == SeatResult::Success);
    EXPECT_EQ(fx.routes.availableSeats("R1").size(), static_cast<std::size_t>(30));
    EXPECT_TRUE(fx.student(id)->seatAssigned() == false);
}

ST_TEST(changeRouteUpdatesTransportFee) {
    SeatFixture fx;
    const std::string id = fx.addStudent("asha", "secret123");
    std::string detail;
    fx.seats.assign(id, "R1", 5, detail);
    EXPECT_EQ(fx.student(id)->transportFee(), 9000LL);

    EXPECT_TRUE(fx.seats.changeRoute(id, "R4", 8, detail) == SeatResult::Success);
    EXPECT_EQ(fx.student(id)->routeId(), std::string("R4"));
    EXPECT_EQ(fx.student(id)->seatNumber(), 8);
    EXPECT_EQ(fx.student(id)->transportFee(), 8000LL);
    EXPECT_TRUE(fx.routes.isSeatFree("R1", 5));
}

ST_TEST(failedRouteChangeKeepsPreviousSeat) {
    SeatFixture fx;
    const std::string first = fx.addStudent("asha", "secret123");
    const std::string second = fx.addStudent("bilal", "secret123");

    std::string detail;
    fx.seats.assign(first, "R2", 4, detail);
    fx.seats.assign(second, "R1", 9, detail);

    // Bilal already holds R1 seat 9 and tries to move onto Asha's seat.
    const SeatResult result = fx.seats.changeRoute(second, "R2", 4, detail);
    EXPECT_TRUE(result == SeatResult::SeatTaken);

    Student* s = fx.student(second);
    EXPECT_EQ(s->routeId(), std::string("R1"));
    EXPECT_EQ(s->seatNumber(), 9);
    EXPECT_TRUE(detail.find("previous seat was kept") != std::string::npos);

    // The failed attempt must not have consumed Asha's seat either.
    EXPECT_EQ(fx.student(first)->seatNumber(), 4);
}

ST_TEST(unknownRouteIsRejected) {
    SeatFixture fx;
    const std::string id = fx.addStudent("asha", "secret123");
    std::string detail;
    EXPECT_TRUE(fx.seats.assign(id, "R99", 1, detail) == SeatResult::RouteNotFound);
}

ST_TEST(seatMapReportsAvailability) {
    SeatFixture fx;
    const std::string id = fx.addStudent("asha", "secret123");
    std::string detail;
    fx.seats.assign(id, "R5", 2, detail);

    const std::vector<Seat> map = fx.seats.seatMap("R5", id);
    EXPECT_EQ(map.size(), static_cast<std::size_t>(20));
    EXPECT_TRUE(map[1].studentId() == id);
    EXPECT_TRUE(map[0].available());
    EXPECT_TRUE(map[1].available() == false);
}

ST_TEST(autoAssignPicksFirstFreeSeat) {
    SeatFixture fx;
    const std::string first = fx.addStudent("asha", "secret123");
    const std::string second = fx.addStudent("bilal", "secret123");

    std::string detail;
    fx.seats.assign(first, "R3", 1, detail);
    fx.seats.autoAssignFirstFree(second, "R3", detail);

    EXPECT_EQ(fx.student(first)->seatNumber(), 1);
    EXPECT_EQ(fx.student(second)->seatNumber(), 2);
}

int main() {
    std::cout << "test_seats\n";
    return ::st::testing::Registry::instance().run() == 0 ? 0 : 1;
}