#pragma once

#include <memorial/runtime/error.hpp>
#include <memorial/runtime/identity.hpp>
#include <memorial/runtime/provenance.hpp>
#include <memorial/runtime/time.hpp>
#include <memorial/storage/adjacency_store.hpp>
#include <memorial/storage/delta_store.hpp>
#include <memorial/storage/node_store.hpp>
#include <memorial/storage/snapshot.hpp>

#include <string_view>

namespace memorial {

[[nodiscard]] constexpr std::string_view version() noexcept { return "0.1.0"; }

} // namespace memorial
