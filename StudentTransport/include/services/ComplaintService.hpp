#pragma once

#include "models/Complaint.hpp"
#include "models/Notice.hpp"
#include "repositories/ComplaintRepository.hpp"
#include "services/AuthService.hpp"

#include <string>
#include <vector>

namespace st {

enum class ComplaintResult { Success, NotFound, EmptyContent, StorageFailure };

const char* complaintResultMessage(ComplaintResult result);

class ComplaintService {
public:
    ComplaintService(CsvComplaintRepository& repo, AuthService& auth)
        : repo_(repo), auth_(auth) {}

    void load();

    const std::vector<Complaint>& cache() const { return complaints_; }
    std::vector<Complaint> openComplaints() const;
    std::vector<Complaint> forStudent(const std::string& studentId) const;
    std::vector<Complaint> byStatus(ComplaintStatus status) const;
    const Complaint* find(const std::string& complaintId) const;

    ComplaintResult submit(const std::string& studentId, const std::string& subject,
                           const std::string& description, std::string& createdId);
    ComplaintResult resolve(const std::string& complaintId, const std::string& resolver,
                            const std::string& response, std::string& message);
    ComplaintResult reopen(const std::string& complaintId, const std::string& resolver,
                           std::string& message);

    int openCount() const;

private:
    CsvComplaintRepository& repo_;
    AuthService& auth_;
    std::vector<Complaint> complaints_;
};

enum class NoticeResult { Success, NotFound, EmptyContent, StorageFailure };

const char* noticeResultMessage(NoticeResult result);

class NoticeService {
public:
    explicit NoticeService(CsvNoticeRepository& repo) : repo_(repo) {}

    void load();

    const std::vector<Notice>& cache() const { return notices_; }
    const Notice* find(const std::string& noticeId) const;

    NoticeResult publish(const std::string& title, const std::string& content,
                         const std::string& authorId, std::string& createdId);
    NoticeResult edit(const std::string& noticeId, const std::string& title,
                      const std::string& content, std::string& message);
    NoticeResult remove(const std::string& noticeId, std::string& message);

private:
    CsvNoticeRepository& repo_;
    std::vector<Notice> notices_;
};

}  // namespace st