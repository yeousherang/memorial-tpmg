#pragma once

#include <memorial/runtime/error.hpp>
#include <memorial/runtime/identity.hpp>

#include <optional>
#include <string>
#include <utility>

namespace memorial {

enum class provenance_kind {
    observed,
    self_reported,
    imported,
    inferred,
    simulated,
};

class provenance {
  public:
    [[nodiscard]] static result<provenance> make(provenance_kind kind, std::string source,
                                                 std::optional<model_run_id> model_run = {}) {
        if (source.empty()) {
            return std::unexpected(
                graph_error{graph_errc::missing_provenance, "provenance source must not be empty"});
        }
        if (requires_model_run(kind) && (!model_run || !model_run->is_valid())) {
            return std::unexpected(
                graph_error{graph_errc::missing_provenance,
                            "inferred and simulated provenance require a valid model run ID"});
        }
        if (model_run && !model_run->is_valid()) {
            return std::unexpected(
                graph_error{graph_errc::invalid_id, "provenance model run ID must be valid"});
        }
        return provenance{kind, std::move(source), model_run};
    }

    [[nodiscard]] provenance_kind kind() const noexcept { return kind_; }
    [[nodiscard]] const std::string& source() const noexcept { return source_; }
    [[nodiscard]] std::optional<model_run_id> model_run() const noexcept { return model_run_; }

  private:
    [[nodiscard]] static constexpr bool requires_model_run(provenance_kind kind) noexcept {
        return kind == provenance_kind::inferred || kind == provenance_kind::simulated;
    }

    provenance(provenance_kind kind, std::string source, std::optional<model_run_id> model_run)
        : kind_{kind}, source_{std::move(source)}, model_run_{model_run} {}

    provenance_kind kind_;
    std::string source_;
    std::optional<model_run_id> model_run_;
};

} // namespace memorial
