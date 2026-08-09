#pragma once

#include <expected>
#include <string>
#include <utility>

namespace memorial {

enum class graph_errc {
    invalid_id,
    id_not_found,
    invalid_interval,
    worldline_mismatch,
    probability_out_of_domain,
    missing_provenance,
    conflict,
    capacity_exceeded,
};

class graph_error {
  public:
    explicit graph_error(graph_errc code, std::string detail = {})
        : code_{code}, detail_{std::move(detail)} {}

    [[nodiscard]] graph_errc code() const noexcept { return code_; }
    [[nodiscard]] const std::string& detail() const noexcept { return detail_; }

    friend bool operator==(const graph_error&, const graph_error&) = default;

  private:
    graph_errc code_;
    std::string detail_;
};

template <typename Type> using result = std::expected<Type, graph_error>;

} // namespace memorial
