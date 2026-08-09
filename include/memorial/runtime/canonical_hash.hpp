#pragma once

#include <memorial/id/strong_id.hpp>
#include <memorial/runtime/event_log.hpp>
#include <memorial/runtime/provenance.hpp>
#include <memorial/runtime/time.hpp>

#include <bit>
#include <cmath>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace memorial {

class canonical_hasher {
  public:
    constexpr void append_byte(std::uint8_t value) noexcept {
        value_ ^= value;
        value_ *= prime;
    }

    void append_bytes(std::string_view bytes) noexcept {
        for (const auto byte : bytes) {
            append_byte(static_cast<std::uint8_t>(static_cast<unsigned char>(byte)));
        }
    }

    void append_tag(std::string_view tag) noexcept {
        append_byte(0xF0U);
        append_unsigned(static_cast<std::uint64_t>(tag.size()));
        append_bytes(tag);
    }

    template <std::unsigned_integral Value> constexpr void append_unsigned(Value value) noexcept {
        static_assert(sizeof(Value) <= sizeof(std::uint64_t),
                      "canonical hashing supports unsigned integers up to 64 bits");
        auto remaining = static_cast<std::uint64_t>(value);
        for (std::size_t index = 0; index < sizeof(Value); ++index) {
            append_byte(static_cast<std::uint8_t>(remaining & 0xFFU));
            remaining >>= 8U;
        }
    }

    [[nodiscard]] constexpr std::uint64_t finish() const noexcept { return value_; }

  private:
    static constexpr std::uint64_t offset_basis = 14695981039346656037ULL;
    static constexpr std::uint64_t prime = 1099511628211ULL;

    std::uint64_t value_{offset_basis};
};

struct canonical_digest {
    std::uint64_t value{};

    [[nodiscard]] std::string hex() const {
        constexpr char digits[] = "0123456789abcdef";
        std::string output(16U, '0');
        auto remaining = value;
        for (auto index = output.size(); index > 0; --index) {
            output[index - 1U] = digits[remaining & 0xFU];
            remaining >>= 4U;
        }
        return output;
    }

    friend constexpr auto operator<=>(canonical_digest, canonical_digest) noexcept = default;
};

inline void canonical_hash_append(canonical_hasher& hasher, bool value) noexcept {
    hasher.append_byte(0x01U);
    hasher.append_byte(value ? 1U : 0U);
}

template <std::integral Value>
    requires(!std::same_as<std::remove_cv_t<Value>, bool>)
void canonical_hash_append(canonical_hasher& hasher, Value value) noexcept {
    using unsigned_type = std::make_unsigned_t<Value>;
    hasher.append_byte(0x02U);
    hasher.append_byte(static_cast<std::uint8_t>(sizeof(Value)));
    hasher.append_unsigned(std::bit_cast<unsigned_type>(value));
}

template <typename Value>
    requires std::is_enum_v<Value>
void canonical_hash_append(canonical_hasher& hasher, Value value) noexcept {
    canonical_hash_append(hasher, static_cast<std::underlying_type_t<Value>>(value));
}

template <std::floating_point Value>
void canonical_hash_append(canonical_hasher& hasher, Value value) noexcept {
    static_assert(std::numeric_limits<Value>::is_iec559,
                  "canonical floating-point hashing requires IEC 559 representation");
    hasher.append_byte(0x03U);
    hasher.append_byte(static_cast<std::uint8_t>(sizeof(Value)));
    if (value == static_cast<Value>(0)) {
        value = static_cast<Value>(0);
    } else if (std::isnan(value)) {
        value = std::numeric_limits<Value>::quiet_NaN();
    }
    if constexpr (sizeof(Value) == sizeof(std::uint32_t)) {
        hasher.append_unsigned(std::bit_cast<std::uint32_t>(value));
    } else if constexpr (sizeof(Value) == sizeof(std::uint64_t)) {
        hasher.append_unsigned(std::bit_cast<std::uint64_t>(value));
    } else {
        static_assert(sizeof(Value) == sizeof(std::uint32_t) ||
                          sizeof(Value) == sizeof(std::uint64_t),
                      "canonical hashing supports 32-bit and 64-bit floating-point values");
    }
}

inline void canonical_hash_append(canonical_hasher& hasher, std::string_view value) noexcept {
    hasher.append_byte(0x04U);
    hasher.append_unsigned(static_cast<std::uint64_t>(value.size()));
    hasher.append_bytes(value);
}

inline void canonical_hash_append(canonical_hasher& hasher, const std::string& value) noexcept {
    canonical_hash_append(hasher, std::string_view{value});
}

template <typename Tag, IdValue Value>
void canonical_hash_append(canonical_hasher& hasher, strong_id<Tag, Value> id) noexcept {
    hasher.append_byte(0x05U);
    hasher.append_unsigned(id.value());
}

inline void canonical_hash_append(canonical_hasher& hasher, timestamp value) noexcept {
    hasher.append_byte(0x06U);
    canonical_hash_append(hasher, value.time_since_epoch().count());
}

template <typename Value>
    requires requires(canonical_hasher& hasher, const Value& value) {
        canonical_hash_append(hasher, value);
    }
void canonical_hash_append(canonical_hasher& hasher, const std::optional<Value>& value) noexcept(
    noexcept(canonical_hash_append(hasher, *value))) {
    hasher.append_byte(0x07U);
    canonical_hash_append(hasher, value.has_value());
    if (value) {
        canonical_hash_append(hasher, *value);
    }
}

inline void canonical_hash_append(canonical_hasher& hasher, const provenance& value) noexcept {
    hasher.append_tag("memorial.provenance.v1");
    canonical_hash_append(hasher, value.kind());
    canonical_hash_append(hasher, value.source());
    canonical_hash_append(hasher, value.model_run());
}

template <typename Value>
concept CanonicallyHashable = requires(canonical_hasher& hasher, const Value& value) {
    canonical_hash_append(hasher, value);
};

template <CanonicallyHashable Value>
[[nodiscard]] canonical_digest canonical_hash_of(const Value& value) noexcept(
    noexcept(canonical_hash_append(std::declval<canonical_hasher&>(), value))) {
    canonical_hasher hasher;
    canonical_hash_append(hasher, value);
    return {hasher.finish()};
}

template <EventPayload Payload, typename PayloadHash>
    requires CanonicallyHashable<Payload>
[[nodiscard]] canonical_digest canonical_hash_of(const event_log<Payload, PayloadHash>& log) {
    canonical_hasher hasher;
    hasher.append_tag("memorial.event_log.v1");
    canonical_hash_append(hasher, static_cast<std::uint64_t>(log.size()));
    for (const auto& event : log.events()) {
        hasher.append_tag("memorial.graph_event.v1");
        canonical_hash_append(hasher, event.sequence());
        canonical_hash_append(hasher, event.kind());
        canonical_hash_append(hasher, event.valid_time());
        canonical_hash_append(hasher, event.recorded_time());
        canonical_hash_append(hasher, event.worldline());
        canonical_hash_append(hasher, event.payload());
        canonical_hash_append(hasher, event.previous());
        canonical_hash_append(hasher, event.source());
    }
    return {hasher.finish()};
}

} // namespace memorial
