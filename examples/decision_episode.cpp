#include <memorial/graph.hpp>
#include <memorial/schema/memorial_schema.hpp>

#include <chrono>
#include <iostream>
#include <utility>

namespace {

using namespace std::chrono_literals;
using graph_store = memorial::delta_store<memorial::memorial_schema>;
using graph_snapshot = memorial::snapshot<memorial::memorial_schema>;

int fail(const memorial::graph_error& error) {
    std::cerr << "episode error (" << static_cast<int>(error.code()) << "): " << error.detail()
              << '\n';
    return 1;
}

} // namespace

int main() {
    const auto valid =
        memorial::valid_interval::make(memorial::timestamp{10ns}, memorial::timestamp{100ns});
    const auto transaction = memorial::transaction_interval::open_ended(memorial::timestamp{20ns});
    const auto source =
        memorial::provenance::make(memorial::provenance_kind::self_reported, "episode journal");
    if (!valid) {
        return fail(valid.error());
    }
    if (!transaction) {
        return fail(transaction.error());
    }
    if (!source) {
        return fail(source.error());
    }

    const memorial::worldline_id actual{1U};
    graph_store delta;
    const auto experience = delta.nodes<memorial::domain::experience_tag>().append(
        actual, *valid, *transaction, *source, 0.95F);
    const auto perception = delta.nodes<memorial::domain::perception_tag>().append(
        actual, *valid, *transaction, *source, 0.90F);
    const auto thought = delta.nodes<memorial::domain::thought_tag>().append(
        actual, *valid, *transaction, *source, 0.80F, 0.85F);
    const auto decision = delta.nodes<memorial::domain::decision_tag>().append(
        actual, *valid, *transaction, *source, 0.88F);
    const auto action = delta.nodes<memorial::domain::action_tag>().append(
        actual, *valid, *transaction, *source, 0.92F);
    const auto outcome = delta.nodes<memorial::domain::outcome_tag>().append(
        actual, *valid, *transaction, *source, 0.78F);
    const auto memory = delta.nodes<memorial::domain::memory_tag>().append(
        actual, *valid, *transaction, *source, 0.70F, 0.82F);
    if (!experience || !perception || !thought || !decision || !action || !outcome || !memory) {
        std::cerr << "episode error: failed to append a node\n";
        return 1;
    }

    const auto generates =
        delta.append_edge<memorial::domain::experience_tag, memorial::domain::generates_relation,
                          memorial::domain::perception_tag>(*experience, *perception, 0.91);
    const auto activates =
        delta.append_edge<memorial::domain::perception_tag, memorial::domain::activates_relation,
                          memorial::domain::thought_tag>(*perception, *thought, 0.84, 0.72);
    const auto selects =
        delta.append_edge<memorial::domain::decision_tag, memorial::domain::selects_relation,
                          memorial::domain::action_tag>(*decision, *action, 0.89);
    const auto causes =
        delta.append_edge<memorial::domain::action_tag, memorial::domain::causes_relation,
                          memorial::domain::outcome_tag>(*action, *outcome, 0.86, 0.76);
    const auto encodes =
        delta.append_edge<memorial::domain::outcome_tag, memorial::domain::encodes_relation,
                          memorial::domain::memory_tag>(*outcome, *memory, 0.81, 0.68);
    const auto recalls =
        delta.append_edge<memorial::domain::memory_tag, memorial::domain::recalls_relation,
                          memorial::domain::experience_tag>(*memory, *experience, 0.74, 0.61);
    if (!generates || !activates || !selects || !causes || !encodes || !recalls) {
        std::cerr << "episode error: failed to append an edge\n";
        return 1;
    }

    const auto published =
        graph_snapshot::publish(memorial::generation_id{1U}, actual, memorial::timestamp{50ns},
                                memorial::timestamp{50ns}, std::move(delta));
    if (!published) {
        return fail(published.error());
    }

    const auto selected_actions =
        published->outgoing<memorial::domain::decision_tag, memorial::domain::selects_relation,
                            memorial::domain::action_tag>(*decision);
    const auto outcome_confidence =
        published->property<memorial::domain::outcome_tag, "confidence">(*outcome);
    if (!selected_actions) {
        return fail(selected_actions.error());
    }
    if (!outcome_confidence) {
        return fail(outcome_confidence.error());
    }

    std::cout << "Decision episode: " << selected_actions->size()
              << " selected action, outcome confidence " << outcome_confidence->get() << '\n';
    return 0;
}
