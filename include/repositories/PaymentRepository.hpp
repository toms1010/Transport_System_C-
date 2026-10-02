#pragma once

#include "models/Payment.hpp"

#include <string>
#include <vector>

namespace st {

class CsvPaymentRepository {
public:
    explicit CsvPaymentRepository(std::string path) : path_(std::move(path)) {}

    std::vector<Payment> loadAll() const;
    void saveAll(const std::vector<Payment>& payments) const;

    bool append(const Payment& payment, std::vector<Payment>& cache) const;
    bool removeById(const std::string& paymentId, std::vector<Payment>& cache) const;

    std::vector<Payment> forStudent(const std::vector<Payment>& cache,
                                    const std::string& studentId) const;
    long long totalForStudent(const std::vector<Payment>& cache, const std::string& studentId,
                              FeeKind kind) const;
    std::string nextId(const std::vector<Payment>& cache) const;

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

}  // namespace st