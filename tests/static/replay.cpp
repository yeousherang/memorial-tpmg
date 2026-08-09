#include <memorial/runtime/replay.hpp>

#include <concepts>
#include <string>

struct state {
    int value{};
};

struct reducer {
    memorial::result<void> operator()(state&, const memorial::graph_event<std::string>&) const;
};

static_assert(memorial::CheckpointState<state>);
static_assert(memorial::ReplayReducer<reducer, state, memorial::graph_event<std::string>>);
static_assert(std::copy_constructible<memorial::checkpoint<state>>);

int main() {}
