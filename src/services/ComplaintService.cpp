#include "services/ComplaintService.hpp"

#include "utils/TextUtils.hpp"
#include "utils/Validation.hpp"

#include <algorithm>

namespace st {

const char* complaintResultMessage(ComplaintResult result) {
    switch (result) {
        case ComplaintResult::Success: return "Complaint updated.";
        case ComplaintResult::NotFound: return "Complaint not found.";
        case ComplaintResult::EmptyContent: return "Complaint content cannot be empty.";
        case ComplaintResult::StorageFailure: return "Could not save the complaint.";
        default: return "Complaint operation failed.";
    }
}

void ComplaintService::load() {
    complaints_ = repo_.loadAll();
}

std::vector<Complaint> ComplaintService::openComplaints() const {
    return byStatus(ComplaintStatus::Open);
}

std::vector<Complaint> ComplaintService::forStudent(const std::string& studentId) const {
    std::vector<Complaint> out;
    for (const Complaint& c : complaints_) {
        if (c.studentId() == studentId) out.push_back(c);
    }
    return out;
}

std::vector<Complaint> ComplaintService::byStatus(ComplaintStatus status) const {
    std::vector<Complaint> out;
    for (const Complaint& c : complaints_) {
        if (c.status() == status) out.push_back(c);
    }
    return out;
}

const Complaint* ComplaintService::find(const std::string& complaintId) const {
    for (const Complaint& c : complaints_) {
        if (c.complaintId() == complaintId) return &c;
    }
    return nullptr;
}

ComplaintResult ComplaintService::submit(const std::string& studentId, const std::string& subject,
                                         const std::string& description,
                                         std::string& createdId) {
    std::string reason;
    if (!validate::complaintSubject(subject, reason) ||
        !validate::complaintDescription(description, reason)) {
        createdId.clear();
        return ComplaintResult::EmptyContent;
    }
    Complaint c(repo_.nextId(complaints_), studentId, util::trim(subject),
                util::trim(description), util::nowStamp());
    if (!repo_.append(c, complaints_)) return ComplaintResult::StorageFailure;
    createdId = c.complaintId();
    return ComplaintResult::Success;
}

ComplaintResult ComplaintService::resolve(const std::string& complaintId,
                                          const std::string& resolver,
                                          const std::string& response, std::string& message) {
    for (Complaint& c : complaints_) {
        if (c.complaintId() != complaintId) continue;
        c.setStatus(ComplaintStatus::Resolved);
        c.setResponse(util::trim(response));
        c.setResolvedAt(util::nowStamp());
        c.setResolvedBy(resolver);
        if (!repo_.update(c, complaints_)) {
            message = complaintResultMessage(ComplaintResult::StorageFailure);
            return ComplaintResult::StorageFailure;
        }
        message = "Complaint " + complaintId + " marked resolved.";
        return ComplaintResult::Success;
    }
    message = complaintResultMessage(ComplaintResult::NotFound);
    return ComplaintResult::NotFound;
}

ComplaintResult ComplaintService::reopen(const std::string& complaintId, const std::string& resolver,
                                         std::string& message) {
    for (Complaint& c : complaints_) {
        if (c.complaintId() != complaintId) continue;
        c.setStatus(ComplaintStatus::Open);
        c.setResponse("");
        c.setResolvedAt("");
        c.setResolvedBy("");
        if (!repo_.update(c, complaints_)) {
            message = complaintResultMessage(ComplaintResult::StorageFailure);
            return ComplaintResult::StorageFailure;
        }
        message = "Complaint " + complaintId + " reopened by " + resolver + ".";
        return ComplaintResult::Success;
    }
    message = complaintResultMessage(ComplaintResult::NotFound);
    return ComplaintResult::NotFound;
}

int ComplaintService::openCount() const {
    int count = 0;
    for (const Complaint& c : complaints_) {
        if (c.status() == ComplaintStatus::Open) ++count;
    }
    return count;
}

const char* noticeResultMessage(NoticeResult result) {
    switch (result) {
        case NoticeResult::Success: return "Notice saved.";
        case NoticeResult::NotFound: return "Notice not found.";
        case NoticeResult::EmptyContent: return "Notice content cannot be empty.";
        case NoticeResult::StorageFailure: return "Could not save the notice.";
        default: return "Notice operation failed.";
    }
}

void NoticeService::load() {
    notices_ = repo_.loadAll();
}

const Notice* NoticeService::find(const std::string& noticeId) const {
    for (const Notice& n : notices_) {
        if (n.noticeId() == noticeId) return &n;
    }
    return nullptr;
}

NoticeResult NoticeService::publish(const std::string& title, const std::string& content,
                                   const std::string& authorId, std::string& createdId) {
    std::string reason;
    if (!validate::noticeTitle(title, reason) || !validate::noticeContent(content, reason)) {
        createdId.clear();
        return NoticeResult::EmptyContent;
    }
    Notice n(repo_.nextId(notices_), util::trim(title), util::trim(content), authorId,
             util::nowStamp());
    if (!repo_.append(n, notices_)) return NoticeResult::StorageFailure;
    createdId = n.noticeId();
    return NoticeResult::Success;
}

NoticeResult NoticeService::edit(const std::string& noticeId, const std::string& title,
                                 const std::string& content, std::string& message) {
    std::string reason;
    if (!validate::noticeTitle(title, reason) || !validate::noticeContent(content, reason)) {
        message = reason;
        return NoticeResult::EmptyContent;
    }
    for (Notice& n : notices_) {
        if (n.noticeId() != noticeId) continue;
        n.setTitle(util::trim(title));
        n.setContent(util::trim(content));
        if (!repo_.update(n, notices_)) {
            message = noticeResultMessage(NoticeResult::StorageFailure);
            return NoticeResult::StorageFailure;
        }
        message = "Notice " + noticeId + " updated.";
        return NoticeResult::Success;
    }
    message = noticeResultMessage(NoticeResult::NotFound);
    return NoticeResult::NotFound;
}

NoticeResult NoticeService::remove(const std::string& noticeId, std::string& message) {
    if (!repo_.removeById(noticeId, notices_)) {
        message = noticeResultMessage(NoticeResult::NotFound);
        return NoticeResult::NotFound;
    }
    message = "Notice " + noticeId + " deleted.";
    return NoticeResult::Success;
}

}  // namespace st