#include <memorial/runtime/canonical_hash.hpp>

#include <concepts>
#include <string>

struct custom_state {
    std::string label;
    int value{};
};

void canonical_hash_append(memorial::canonical_hasher& hasher, const custom_state& state) noexcept {
    hasher.append_tag("test.custom_state.v1");
    memorial::canonical_hash_append(hasher, state.label);
    memorial::canonical_hash_append(hasher, state.value);
}

static_assert(memorial::CanonicallyHashable<int>);
static_assert(memorial::CanonicallyHashable<std::string>);
static_assert(memorial::CanonicallyHashable<custom_state>);
static_assert(std::same_as<decltype(memorial::canonical_hash_of(custom_state{})),
                           memorial::canonical_digest>);

int main() {}
