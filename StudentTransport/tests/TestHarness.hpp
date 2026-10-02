#pragma once

#include <functional>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

namespace st {
namespace testing {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

class Registry {
public:
    static Registry& instance() {
        static Registry registry;
        return registry;
    }

    void add(const std::string& name, std::function<void()> body) {
        cases_.push_back(TestCase{name, std::move(body)});
    }

    int run() const {
        int failed = 0;
        for (const TestCase& c : cases_) {
            try {
                c.body();
                std::cout << "  [PASS] " << c.name << "\n";
            } catch (const std::exception& e) {
                std::cout << "  [FAIL] " << c.name << "\n         " << e.what() << "\n";
                ++failed;
            }
        }
        std::cout << "\n" << (cases_.size() - static_cast<std::size_t>(failed)) << "/"
                  << cases_.size() << " passed\n";
        return failed;
    }

private:
    std::vector<TestCase> cases_;
};

struct Registrar {
    Registrar(const std::string& name, std::function<void()> body) {
        Registry::instance().add(name, std::move(body));
    }
};

class AssertionFailure : public std::runtime_error {
public:
    explicit AssertionFailure(const std::string& what) : std::runtime_error(what) {}
};

// Compares without tripping -Wsign-compare on mixed size_t/int expectations,
// while still supporting string comparisons.
template <typename T>
struct IsIntegral : std::false_type {};
template <> struct IsIntegral<int> : std::true_type {};
template <> struct IsIntegral<unsigned int> : std::true_type {};
template <> struct IsIntegral<long> : std::true_type {};
template <> struct IsIntegral<unsigned long> : std::true_type {};
template <> struct IsIntegral<long long> : std::true_type {};
template <> struct IsIntegral<unsigned long long> : std::true_type {};
template <> struct IsIntegral<bool> : std::true_type {};
template <> struct IsIntegral<char> : std::true_type {};

template <typename A, typename B>
bool valuesMatch(const A& actual, const B& expected) {
    if constexpr (IsIntegral<A>::value && IsIntegral<B>::value) {
        return static_cast<long long>(actual) == static_cast<long long>(expected);
    } else {
        return actual == expected;
    }
}

template <typename A, typename B>
void expectEqual(const A& actual, const B& expected, const char* expr, const char* file, int line) {
    if (!valuesMatch(actual, expected)) {
        std::ostringstream oss;
        oss << "expected " << expr << "\n         at " << file << ":" << line;
        throw AssertionFailure(oss.str());
    }
}

inline void expectTrue(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        std::ostringstream oss;
        oss << "expected true: " << expr << "\n         at " << file << ":" << line;
        throw AssertionFailure(oss.str());
    }
}

}
}

#define ST_TEST(name)                                                                       \
    static void name();                                                                     \
    static ::st::testing::Registrar registrar_##name(#name, name);                          \
    static void name()

#define EXPECT_EQ(actual, expected) \
    ::st::testing::expectEqual((actual), (expected), #actual " == " #expected, __FILE__, __LINE__)

#define EXPECT_TRUE(cond) ::st::testing::expectTrue((cond), #cond, __FILE__, __LINE__)

#define ST_TEST_MAIN()                                            \
    int main() { return ::st::testing::Registry::instance().run() == 0 ? 0 : 1; }
