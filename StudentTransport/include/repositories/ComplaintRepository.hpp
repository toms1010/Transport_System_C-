#pragma once

#include "models/Complaint.hpp"
#include "models/Notice.hpp"

#include <string>
#include <vector>

namespace st {

class CsvComplaintRepository {
public:
    explicit CsvComplaintRepository(std::string path) : path_(std::move(path)) {}

    std::vector<Complaint> loadAll() const;
    void saveAll(const std::vector<Complaint>& complaints) const;

    bool append(const Complaint& complaint, std::vector<Complaint>& cache) const;
    bool update(const Complaint& complaint, std::vector<Complaint>& cache) const;
    bool removeById(const std::string& complaintId, std::vector<Complaint>& cache) const;

    std::string nextId(const std::vector<Complaint>& cache) const;

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

class CsvNoticeRepository {
public:
    explicit CsvNoticeRepository(std::string path) : path_(std::move(path)) {}

    std::vector<Notice> loadAll() const;
    void saveAll(const std::vector<Notice>& notices) const;

    bool append(const Notice& notice, std::vector<Notice>& cache) const;
    bool update(const Notice& notice, std::vector<Notice>& cache) const;
    bool removeById(const std::string& noticeId, std::vector<Notice>& cache) const;

    std::string nextId(const std::vector<Notice>& cache) const;

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

}  // namespace st