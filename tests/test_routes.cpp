#include "TestHarness.hpp"

#include "repositories/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "services/RouteService.hpp"
#include "utils/Validation.hpp"

#include <filesystem>

using namespace st;

namespace {

struct RouteFixture {
    std::filesystem::path dir;
    CsvUserRepository users;
    CsvRouteRepository routesRepo;
    CsvPaymentRepository paymentsRepo;
    ConfigService config;
    AuthService auth{users, paymentsRepo, config};
    RouteService routes;

    RouteFixture()
        : dir(std::filesystem::temp_directory_path() / "st_test_routes"),
          users((dir / "users.csv").string()),
          routesRepo((dir / "routes.csv").string()),
          paymentsRepo((dir / "payments.csv").string()),
          config(),
          auth(users, paymentsRepo, config),
          routes(auth, routesRepo) {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        config.load((dir / "config.csv").string());
        routes.load();
    }

    ~RouteFixture() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
};

}

ST_TEST(defaultRoutesAreSeeded) {
    RouteFixture fx;
    EXPECT_EQ(fx.routes.cache().size(), static_cast<std::size_t>(5));
    EXPECT_TRUE(fx.routes.routeExists("R1"));
    EXPECT_TRUE(fx.routes.routeExists("R5"));
}

ST_TEST(seededRoutesMatchSpecification) {
    RouteFixture fx;
    const Route* r1 = fx.routes.find("R1");
    EXPECT_TRUE(r1 != nullptr);
    EXPECT_EQ(r1->fare(), 9000LL);
    EXPECT_EQ(r1->capacity(), 30);
    EXPECT_EQ(r1->via(), std::string("Kachiguda, Secunderabad"));

    const Route* r3 = fx.routes.find("R3");
    EXPECT_TRUE(r3 != nullptr);
    EXPECT_EQ(r3->fare(), 9500LL);
    EXPECT_EQ(r3->capacity(), 25);
}

ST_TEST(routeLookupFindsById) {
    RouteFixture fx;
    const Route* r2 = fx.routes.find("R2");
    EXPECT_TRUE(r2 != nullptr);
    EXPECT_EQ(r2->routeId(), std::string("R2"));
    EXPECT_EQ(r2->endpoints(), std::string("Gachibowli -> Secunderabad"));
}

ST_TEST(unknownRouteReturnsNull) {
    RouteFixture fx;
    EXPECT_TRUE(fx.routes.find("R99") == nullptr);
    EXPECT_TRUE(fx.routes.routeExists("R99") == false);
}

ST_TEST(addRoutePersistsAndIsRetrievable) {
    RouteFixture fx;
    std::string reason;
    const Route fresh("R6", "Kondapur - Nizampet", "Kondapur", "Nizampet", "Gachibowli", 7200,
                      24);
    EXPECT_TRUE(fx.routes.addRoute(fresh, reason));
    EXPECT_TRUE(fx.routes.routeExists("R6"));

    RouteService reloaded(fx.auth, fx.routesRepo);
    reloaded.load();
    EXPECT_TRUE(reloaded.find("R6") != nullptr);
    EXPECT_EQ(reloaded.find("R6")->fare(), 7200LL);
}

ST_TEST(duplicateRouteIdIsRejected) {
    RouteFixture fx;
    std::string reason;
    const Route dupe("R1", "Duplicate", "A", "B", "C", 1000, 10);
    EXPECT_TRUE(fx.routes.addRoute(dupe, reason) == false);
    EXPECT_TRUE(reason.find("already exists") != std::string::npos);
}

ST_TEST(invalidRouteDataIsRejected) {
    RouteFixture fx;
    std::string reason;
    EXPECT_TRUE(fx.routes.addRoute(Route("", "No id", "A", "B", "C", 1000, 10), reason) == false);
    EXPECT_TRUE(fx.routes.addRoute(Route("R7", "Bad fare", "A", "B", "C", -50, 10), reason) ==
                false);
    EXPECT_TRUE(fx.routes.addRoute(Route("R8", "Bad capacity", "A", "B", "C", 1000, 0), reason) ==
                false);
}

ST_TEST(routeWithStudentsCannotBeDeleted) {
    RouteFixture fx;
    RegistrationForm form;
    form.fullName = "Asha Rao";
    form.fatherName = "Kiran";
    form.phone = "9876543210";
    form.address = "Hostel";
    form.username = "asha";
    form.password = "secret123";

    fx.auth.loadUsers();
    fx.auth.registerStudent(form);

    for (auto& user : fx.auth.userCache()) {
        auto* s = dynamic_cast<Student*>(user.get());
        if (!s) continue;
        s->setRouteId("R1");
        s->setSeatNumber(3);
        fx.users.update(*s, fx.auth.userCache());
    }

    EXPECT_EQ(fx.routes.studentCountOnRoute("R1"), static_cast<std::size_t>(1));

    std::string reason;
    EXPECT_TRUE(fx.routes.deleteRoute("R1", reason) == false);
    EXPECT_TRUE(reason.find("Reassign") != std::string::npos);
}

ST_TEST(emptyRouteCanBeDeleted) {
    RouteFixture fx;
    std::string reason;
    EXPECT_TRUE(fx.routes.deleteRoute("R5", reason));
    EXPECT_TRUE(fx.routes.routeExists("R5") == false);
}

ST_TEST(routeIdValidation) {
    std::string reason;
    EXPECT_TRUE(validate::routeId("R1", reason));
    EXPECT_TRUE(validate::routeId("", reason) == false);
    EXPECT_TRUE(validate::routeId("R 1", reason) == false);
    EXPECT_TRUE(validate::routeId(std::string(20, 'X'), reason) == false);
}

int main() {
    std::cout << "test_routes\n";
    return ::st::testing::Registry::instance().run() == 0 ? 0 : 1;
}