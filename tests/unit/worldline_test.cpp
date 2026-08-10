#include <memorial/runtime/worldline.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <string>

namespace {

using namespace std::chrono_literals;
using log_type = memorial::event_log<std::string>;

memorial::provenance observed_source() {
    return *memorial::provenance::make(memorial::provenance_kind::observed, "actual history");
}

memorial::provenance simulated_source() {
    return *memorial::provenance::make(memorial::provenance_kind::simulated, "model registry",
                                       memorial::model_run_id{3U});
}

memorial::event_sequence append_fork_event(log_type& log, memorial::worldline_id parent,
                                           int valid_nanoseconds, int recorded_nanoseconds) {
    return *log.append(memorial::event_kind::worldline_forked,
                       memorial::timestamp{std::chrono::nanoseconds{valid_nanoseconds}},
                       memorial::timestamp{std::chrono::nanoseconds{recorded_nanoseconds}}, parent,
                       "replace selected action", observed_source());
}

TEST(WorldlineRegistry, CreatesActualRootAndForksSimulatedChild) {
    memorial::worldline_registry registry;
    const auto root = registry.create_root(memorial::generation_id{1U}, observed_source());
    ASSERT_TRUE(root);
    log_type log;
    const auto fork_event = append_fork_event(log, *root, 50, 60);

    const auto child = registry.fork(*root, log, fork_event, memorial::intervention_id{4U},
                                     memorial::generation_id{2U}, simulated_source());

    ASSERT_TRUE(child);
    const auto record = registry.find(*child);
    ASSERT_TRUE(record);
    EXPECT_EQ(record->get().kind(), memorial::worldline_kind::simulated);
    EXPECT_EQ(record->get().parent(), *root);
    EXPECT_EQ(record->get().fork_event(), fork_event);
    EXPECT_EQ(record->get().fork_valid_time(), memorial::timestamp{50ns});
    EXPECT_EQ(record->get().intervention(), memorial::intervention_id{4U});
}

TEST(WorldlineRegistry, ResolvesParentAndRootToChildLineage) {
    memorial::worldline_registry registry;
    const auto root = registry.create_root(memorial::generation_id{1U}, observed_source());
    ASSERT_TRUE(root);
    log_type log;
    const auto first_event = append_fork_event(log, *root, 50, 60);
    const auto child = registry.fork(*root, log, first_event, memorial::intervention_id{4U},
                                     memorial::generation_id{2U}, simulated_source());
    ASSERT_TRUE(child);
    const auto second_event = append_fork_event(log, *child, 70, 80);
    const auto grandchild = registry.fork(*child, log, second_event, memorial::intervention_id{5U},
                                          memorial::generation_id{3U}, simulated_source());
    ASSERT_TRUE(grandchild);

    const auto parent = registry.parent_of(*grandchild);
    const auto lineage = registry.lineage(*grandchild);

    ASSERT_TRUE(parent);
    ASSERT_TRUE(lineage);
    EXPECT_EQ(parent->get().id(), *child);
    EXPECT_EQ(*lineage, (std::vector<memorial::worldline_id>{*root, *child, *grandchild}));
}

TEST(WorldlineRegistry, RecordReferencesSurviveLaterForks) {
    memorial::worldline_registry registry;
    const auto root = registry.create_root(memorial::generation_id{1U}, observed_source());
    ASSERT_TRUE(root);
    const auto original_reference = registry.find(*root);
    ASSERT_TRUE(original_reference);
    const auto* original_address = &original_reference->get();
    log_type log;

    for (std::uint64_t index = 0; index < 32U; ++index) {
        const auto event = append_fork_event(log, *root, static_cast<int>(50U + index),
                                             static_cast<int>(60U + index));
        ASSERT_TRUE(registry.fork(*root, log, event, memorial::intervention_id{index + 1U},
                                  memorial::generation_id{index + 2U}, simulated_source()));
    }

    EXPECT_EQ(&registry.find(*root)->get(), original_address);
    EXPECT_EQ(original_reference->get().kind(), memorial::worldline_kind::actual);
}

TEST(WorldlineRegistry, RejectsWrongEventKindAndWorldline) {
    memorial::worldline_registry registry;
    const auto root = registry.create_root(memorial::generation_id{1U}, observed_source());
    ASSERT_TRUE(root);
    log_type log;
    const auto wrong_kind =
        *log.append(memorial::event_kind::state_appended, memorial::timestamp{50ns},
                    memorial::timestamp{60ns}, *root, "state", observed_source());
    const auto kind_result = registry.fork(*root, log, wrong_kind, memorial::intervention_id{1U},
                                           memorial::generation_id{2U}, simulated_source());
    ASSERT_FALSE(kind_result);
    EXPECT_EQ(kind_result.error().code(), memorial::graph_errc::conflict);

    const auto other_parent = memorial::worldline_id{8U};
    const auto wrong_parent = append_fork_event(log, other_parent, 70, 80);
    const auto parent_result =
        registry.fork(*root, log, wrong_parent, memorial::intervention_id{1U},
                      memorial::generation_id{2U}, simulated_source());
    ASSERT_FALSE(parent_result);
    EXPECT_EQ(parent_result.error().code(), memorial::graph_errc::worldline_mismatch);
}

TEST(WorldlineRegistry, EnforcesActualAndSimulatedProvenance) {
    memorial::worldline_registry invalid_root_registry;
    const auto invalid_root =
        invalid_root_registry.create_root(memorial::generation_id{1U}, simulated_source());
    ASSERT_FALSE(invalid_root);

    memorial::worldline_registry registry;
    const auto root = registry.create_root(memorial::generation_id{1U}, observed_source());
    ASSERT_TRUE(root);
    log_type log;
    const auto event = append_fork_event(log, *root, 50, 60);
    const auto child = registry.fork(*root, log, event, memorial::intervention_id{1U},
                                     memorial::generation_id{2U}, observed_source());

    ASSERT_FALSE(child);
    EXPECT_EQ(child.error().code(), memorial::graph_errc::missing_provenance);
}

} // namespace
