#pragma once

#include <memorial/id/strong_id.hpp>

#include <cstdint>

namespace memorial {

struct worldline_id_tag {};
struct generation_id_tag {};
struct model_run_id_tag {};
struct event_sequence_tag {};

using worldline_id = strong_id<worldline_id_tag, std::uint64_t>;
using generation_id = strong_id<generation_id_tag, std::uint64_t>;
using model_run_id = strong_id<model_run_id_tag, std::uint64_t>;
using event_sequence = strong_id<event_sequence_tag, std::uint64_t>;

} // namespace memorial
