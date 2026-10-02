#pragma once

#include <string>

namespace st {

enum class SeatStatus { Available, Occupied, Selected };

const char* seatStatusName(SeatStatus status);

class Seat {
public:
    Seat() = default;
    Seat(std::string routeId, int seatNumber, std::string studentId)
        : routeId_(std::move(routeId)),
          seatNumber_(seatNumber),
          studentId_(std::move(studentId)) {}

    const std::string& routeId() const { return routeId_; }
    int seatNumber() const { return seatNumber_; }
    const std::string& studentId() const { return studentId_; }
    SeatStatus status() const { return seatStatusNameImpl(); }

    // A seat is SELECTED (highlighted) when the viewer is the one holding it.
    SeatStatus statusFor(const std::string& viewerId) const {
        if (studentId_.empty()) return SeatStatus::Available;
        return (!viewerId.empty() && studentId_ == viewerId) ? SeatStatus::Selected
                                                             : SeatStatus::Occupied;
    }
    bool available() const { return studentId_.empty(); }

    void setRouteId(std::string v) { routeId_ = std::move(v); }
    void setSeatNumber(int v) { seatNumber_ = v; }
    void setStudentId(std::string v) { studentId_ = std::move(v); }
    void release() { studentId_.clear(); }

    std::string label() const;
    std::string markerFor(const std::string& viewerId) const;

private:
    SeatStatus seatStatusNameImpl() const {
        return studentId_.empty() ? SeatStatus::Available : SeatStatus::Occupied;
    }

    std::string routeId_;
    int seatNumber_ = 0;
    std::string studentId_;
};

}  // namespace st