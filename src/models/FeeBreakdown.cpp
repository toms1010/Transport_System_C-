#include "models/FeeBreakdown.hpp"
#include "models/Seat.hpp"

namespace st {

const char* feeKindName(FeeKind kind) {
    return kind == FeeKind::Registration ? "REGISTRATION" : "TRANSPORT";
}

FeeKind feeKindFromName(const std::string& name) {
    return name == "REGISTRATION" ? FeeKind::Registration : FeeKind::Transport;
}

std::string FeeBreakdown::statusLabel() const {
    if (registrationFee == 0 && transportFee == 0) return "NOT_APPLICABLE";
    if (outstanding() == 0) return "PAID";
    if (totalPaid() == 0) return "PENDING";
    return "PARTIALLY_PAID";
}

const char* seatStatusName(SeatStatus status) {
    switch (status) {
        case SeatStatus::Occupied: return "OCCUPIED";
        case SeatStatus::Selected: return "SELECTED";
        case SeatStatus::Available:
        default: return "AVAILABLE";
    }
}

std::string Seat::markerFor(const std::string& viewerId) const {
    switch (statusFor(viewerId)) {
        case SeatStatus::Selected: return label() + "\xE2\x9C\x93";
        case SeatStatus::Occupied: return "[X ]";
        case SeatStatus::Available:
        default: return label();
    }
}

std::string Seat::label() const {
    std::string out = "[";
    if (seatNumber_ < 10) out += "0";
    out += std::to_string(seatNumber_);
    out += "]";
    return out;
}

}  // namespace st