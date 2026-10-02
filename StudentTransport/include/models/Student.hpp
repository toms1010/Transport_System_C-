#pragma once

#include "models/FeeBreakdown.hpp"
#include "models/User.hpp"

#include <memory>

namespace st {

// A Student is a User that is enrolled in transport services.
//
// Money model (fixed relative to the original single-`due` design):
//   totalAmount() = registrationFee + transportFee
//   outstanding() = totalAmount() - (registrationPaid + transportPaid)
//
// Registration and transport balances are tracked SEPARATELY so that a payment
// can be applied to the correct bucket, and so that staff reports never re-add
// a registration fee that has already been settled.
class Student : public User {
public:
    Student() { role_ = Role::Student; }

    Student(std::string userId, std::string username, std::string fullName)
        : User(std::move(userId), std::move(username), Role::Student, std::move(fullName)) {}

    const std::string& routeId() const { return routeId_; }
    int seatNumber() const { return seatNumber_; }
    bool seatAssigned() const { return !routeId_.empty() && seatNumber_ > 0; }

    long long registrationFee() const { return registrationFee_; }
    long long registrationPaid() const { return registrationPaid_; }
    long long transportFee() const { return transportFee_; }
    long long transportPaid() const { return transportPaid_; }

    FeeBreakdown breakdown() const {
        FeeBreakdown b;
        b.registrationFee = registrationFee_;
        b.registrationPaid = registrationPaid_;
        b.transportFee = transportFee_;
        b.transportPaid = transportPaid_;
        return b;
    }

    long long totalAmount() const { return registrationFee_ + transportFee_; }
    long long totalPaid() const { return registrationPaid_ + transportPaid_; }
    long long outstanding() const {
        const long long left = totalAmount() - totalPaid();
        return left > 0 ? left : 0;
    }
    bool fullyPaid() const { return outstanding() == 0; }

    void setRouteId(std::string v) { routeId_ = std::move(v); }
    void setSeatNumber(int v) { seatNumber_ = v; }
    void setRegistrationFee(long long v) { registrationFee_ = v < 0 ? 0 : v; }
    void setRegistrationPaid(long long v) { registrationPaid_ = v < 0 ? 0 : v; }
    void setTransportFee(long long v) { transportFee_ = v < 0 ? 0 : v; }
    void setTransportPaid(long long v) { transportPaid_ = v < 0 ? 0 : v; }

    void releaseSeat() {
        routeId_.clear();
        seatNumber_ = 0;
    }

    void addRegistrationPaid(long long amount) {
        registrationPaid_ += amount;
        if (registrationPaid_ > registrationFee_) registrationPaid_ = registrationFee_;
    }

    void addTransportPaid(long long amount) {
        transportPaid_ += amount;
        if (transportPaid_ > transportFee_) transportPaid_ = transportFee_;
    }

    // Keeps running balances inside the current fees after a fare change.
    void clampPaidToFees() {
        if (registrationPaid_ > registrationFee_) registrationPaid_ = registrationFee_;
        if (transportPaid_ > transportFee_) transportPaid_ = transportFee_;
    }

    void setTransportFeeFromFare(long long fare) {
        const bool hadOutstandingTransport = transportFee_ > transportPaid_;
        transportFee_ = fare < 0 ? 0 : fare;
        if (hadOutstandingTransport && transportPaid_ > transportFee_) {
            transportPaid_ = transportFee_;
        }
    }

    std::string roleLabel() const override { return "STUDENT"; }

    std::unique_ptr<User> clone() const override {
        auto copy = std::make_unique<Student>(*this);
        return copy;
    }

private:
    std::string routeId_;
    int seatNumber_ = 0;
    long long registrationFee_ = 0;
    long long registrationPaid_ = 0;
    long long transportFee_ = 0;
    long long transportPaid_ = 0;
};

}  // namespace st