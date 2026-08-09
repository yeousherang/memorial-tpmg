#pragma once

#include <memorial/storage/node_store.hpp>

#include <string_view>

namespace memorial {

[[nodiscard]] constexpr std::string_view version() noexcept { return "0.1.0"; }

} // namespace memorial
