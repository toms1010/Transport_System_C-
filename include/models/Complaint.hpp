#pragma once

#include <string>
#include <vector>

namespace st {

enum class ComplaintStatus { Open, Resolved };

const char* complaintStatusName(ComplaintStatus status);
ComplaintStatus complaintStatusFromName(const std::string& name);

class Complaint {
public:
    Complaint() = default;

    Complaint(std::string complaintId, std::string studentId, std::string subject,
              std::string description, std::string createdAt)
        : complaintId_(std::move(complaintId)),
          studentId_(std::move(studentId)),
          subject_(std::move(subject)),
          description_(std::move(description)),
          createdAt_(std::move(createdAt)) {}

    const std::string& complaintId() const { return complaintId_; }
    const std::string& studentId() const { return studentId_; }
    const std::string& subject() const { return subject_; }
    const std::string& description() const { return description_; }
    const std::string& createdAt() const { return createdAt_; }
    ComplaintStatus status() const { return status_; }
    const std::string& response() const { return response_; }
    const std::string& resolvedAt() const { return resolvedAt_; }
    const std::string& resolvedBy() const { return resolvedBy_; }

    void setComplaintId(std::string v) { complaintId_ = std::move(v); }
    void setStudentId(std::string v) { studentId_ = std::move(v); }
    void setSubject(std::string v) { subject_ = std::move(v); }
    void setDescription(std::string v) { description_ = std::move(v); }
    void setCreatedAt(std::string v) { createdAt_ = std::move(v); }
    void setStatus(ComplaintStatus v) { status_ = v; }
    void setResponse(std::string v) { response_ = std::move(v); }
    void setResolvedAt(std::string v) { resolvedAt_ = std::move(v); }
    void setResolvedBy(std::string v) { resolvedBy_ = std::move(v); }

    std::string statusLabel() const { return complaintStatusName(status_); }

private:
    std::string complaintId_;
    std::string studentId_;
    std::string subject_;
    std::string description_;
    std::string createdAt_;
    ComplaintStatus status_ = ComplaintStatus::Open;
    std::string response_;
    std::string resolvedAt_;
    std::string resolvedBy_;
};

}  // namespace st