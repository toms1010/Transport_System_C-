#include "repositories/PaymentRepository.hpp"

#include "utils/CsvUtils.hpp"
#include "utils/FileUtils.hpp"
#include "utils/TextUtils.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace st {

std::vector<Payment> CsvPaymentRepository::loadAll() const {
    std::vector<Payment> out;
    for (const std::string& line : readLines(path_)) {
        const std::vector<std::string> f = csvSplit(line);
        if (f.size() < 7) continue;
        Payment p;
        p.setPaymentId(f[0]);
        p.setStudentId(f[1]);
        long long amount = 0;
        util::parseWholeNumber(f[2], amount);
        p.setAmount(amount < 0 ? 0 : amount);
        p.setKind(feeKindFromName(f[3]));
        p.setPaymentDate(f[4]);
        p.setReference(f[5]);
        p.setRecordedBy(f[6]);
        out.push_back(p);
    }
    return out;
}

void CsvPaymentRepository::saveAll(const std::vector<Payment>& payments) const {
    std::vector<std::string> lines;
    lines.reserve(payments.size());
    for (const Payment& p : payments) {
        lines.push_back(csvJoin({p.paymentId(), p.studentId(), std::to_string(p.amount()),
                                 p.kindLabel(), p.paymentDate(), p.reference(), p.recordedBy()}));
    }
    writeLines(path_, lines);
}

bool CsvPaymentRepository::append(const Payment& payment, std::vector<Payment>& cache) const {
    for (const Payment& p : cache) {
        if (p.paymentId() == payment.paymentId()) return false;
    }
    cache.push_back(payment);
    saveAll(cache);
    return true;
}

bool CsvPaymentRepository::removeById(const std::string& paymentId,
                                      std::vector<Payment>& cache) const {
    const auto it = std::find_if(cache.begin(), cache.end(),
                                 [&](const Payment& p) { return p.paymentId() == paymentId; });
    if (it == cache.end()) return false;
    cache.erase(it);
    saveAll(cache);
    return true;
}

std::vector<Payment> CsvPaymentRepository::forStudent(const std::vector<Payment>& cache,
                                                      const std::string& studentId) const {
    std::vector<Payment> out;
    for (const Payment& p : cache) {
        if (p.studentId() == studentId) out.push_back(p);
    }
    return out;
}

long long CsvPaymentRepository::totalForStudent(const std::vector<Payment>& cache,
                                                const std::string& studentId,
                                                FeeKind kind) const {
    long long total = 0;
    for (const Payment& p : cache) {
        if (p.studentId() == studentId && p.kind() == kind) total += p.amount();
    }
    return total;
}

std::string CsvPaymentRepository::nextId(const std::vector<Payment>& cache) const {
    int maxId = 0;
    for (const Payment& p : cache) {
        if (util::startsWith(p.paymentId(), "PAY")) {
            long long v = 0;
            util::parseWholeNumber(p.paymentId().substr(3), v);
            if (v > maxId) maxId = static_cast<int>(v > 0 ? v : 0);
        }
    }
    std::ostringstream out;
    out << "PAY" << std::setw(5) << std::setfill('0') << (maxId + 1);
    return out.str();
}

}  // namespace st