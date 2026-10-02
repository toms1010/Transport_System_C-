#include "repositories/ComplaintRepository.hpp"

#include "utils/CsvUtils.hpp"
#include "utils/FileUtils.hpp"
#include "utils/TextUtils.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace st {
namespace {

std::string encodeComplaint(const Complaint& c) {
    return csvJoin({c.complaintId(), c.studentId(), c.subject(), c.description(), c.createdAt(),
                    complaintStatusName(c.status()), c.response(), c.resolvedAt(), c.resolvedBy()});
}

bool decodeComplaint(const std::vector<std::string>& f, Complaint& out) {
    if (f.size() < 9) return false;
    out.setComplaintId(f[0]);
    out.setStudentId(f[1]);
    out.setSubject(f[2]);
    out.setDescription(f[3]);
    out.setCreatedAt(f[4]);
    out.setStatus(complaintStatusFromName(f[5]));
    out.setResponse(f[6]);
    out.setResolvedAt(f[7]);
    out.setResolvedBy(f[8]);
    return true;
}

}

std::vector<Complaint> CsvComplaintRepository::loadAll() const {
    std::vector<Complaint> out;
    for (const std::string& line : readLines(path_)) {
        const std::vector<std::string> f = csvSplit(line);
        Complaint c;
        if (decodeComplaint(f, c)) out.push_back(c);
    }
    return out;
}

void CsvComplaintRepository::saveAll(const std::vector<Complaint>& complaints) const {
    std::vector<std::string> lines;
    lines.reserve(complaints.size());
    for (const Complaint& c : complaints) lines.push_back(encodeComplaint(c));
    writeLines(path_, lines);
}

bool CsvComplaintRepository::append(const Complaint& complaint,
                                    std::vector<Complaint>& cache) const {
    for (const Complaint& c : cache) {
        if (c.complaintId() == complaint.complaintId()) return false;
    }
    cache.push_back(complaint);
    saveAll(cache);
    return true;
}

bool CsvComplaintRepository::update(const Complaint& complaint,
                                    std::vector<Complaint>& cache) const {
    for (Complaint& c : cache) {
        if (c.complaintId() == complaint.complaintId()) {
            c = complaint;
            saveAll(cache);
            return true;
        }
    }
    return false;
}

bool CsvComplaintRepository::removeById(const std::string& complaintId,
                                        std::vector<Complaint>& cache) const {
    const auto it = std::find_if(cache.begin(), cache.end(), [&](const Complaint& c) {
        return c.complaintId() == complaintId;
    });
    if (it == cache.end()) return false;
    cache.erase(it);
    saveAll(cache);
    return true;
}

std::string CsvComplaintRepository::nextId(const std::vector<Complaint>& cache) const {
    int maxId = 0;
    for (const Complaint& c : cache) {
        if (util::startsWith(c.complaintId(), "CMP")) {
            long long n = 0;
            util::parseWholeNumber(c.complaintId().substr(3), n);
            if (n > maxId) maxId = static_cast<int>(n > 0 ? n : 0);
        }
    }
    std::ostringstream out;
    out << "CMP" << std::setw(4) << std::setfill('0') << (maxId + 1);
    return out.str();
}

std::vector<Notice> CsvNoticeRepository::loadAll() const {
    std::vector<Notice> out;
    for (const std::string& line : readLines(path_)) {
        const std::vector<std::string> f = csvSplit(line);
        if (f.size() < 5) continue;
        out.push_back(Notice(f[0], f[1], f[2], f[3], f[4]));
    }
    return out;
}

void CsvNoticeRepository::saveAll(const std::vector<Notice>& notices) const {
    std::vector<std::string> lines;
    lines.reserve(notices.size());
    for (const Notice& n : notices) {
        lines.push_back(csvJoin({n.noticeId(), n.title(), n.content(), n.authorId(), n.createdAt()}));
    }
    writeLines(path_, lines);
}

bool CsvNoticeRepository::append(const Notice& notice, std::vector<Notice>& cache) const {
    for (const Notice& n : cache) {
        if (n.noticeId() == notice.noticeId()) return false;
    }
    cache.push_back(notice);
    saveAll(cache);
    return true;
}

bool CsvNoticeRepository::update(const Notice& notice, std::vector<Notice>& cache) const {
    for (Notice& n : cache) {
        if (n.noticeId() == notice.noticeId()) {
            n = notice;
            saveAll(cache);
            return true;
        }
    }
    return false;
}

bool CsvNoticeRepository::removeById(const std::string& noticeId,
                                     std::vector<Notice>& cache) const {
    const auto it = std::find_if(cache.begin(), cache.end(),
                                 [&](const Notice& n) { return n.noticeId() == noticeId; });
    if (it == cache.end()) return false;
    cache.erase(it);
    saveAll(cache);
    return true;
}

std::string CsvNoticeRepository::nextId(const std::vector<Notice>& cache) const {
    int maxId = 0;
    for (const Notice& n : cache) {
        if (util::startsWith(n.noticeId(), "NOT")) {
            long long v = 0;
            util::parseWholeNumber(n.noticeId().substr(3), v);
            if (v > maxId) maxId = static_cast<int>(v > 0 ? v : 0);
        }
    }
    std::ostringstream out;
    out << "NOT" << std::setw(3) << std::setfill('0') << (maxId + 1);
    return out.str();
}

}  // namespace st