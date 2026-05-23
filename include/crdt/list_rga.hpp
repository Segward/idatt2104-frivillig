#ifndef LIST_RGA_HPP
#define LIST_RGA_HPP

#include <crdt/list.hpp>

// Replicated Growable Array (RGA) for an ordered list of items. Lets multiple
// clients insert and delete entries concurrently and still converge on the
// same order, without a central lock. Each insert is causally chained to its
// predecessor; operations that arrive before their dependency are buffered
// and retried once it lands.
class ListRGA {
private:
    std::string _client_id;
    std::uint64_t _local_sequence = 0;

    std::unordered_map<std::string, ListItem> _nodes;
    std::unordered_map<std::string, std::vector<std::string>> _children;
    std::unordered_set<std::string> _applied_operations;

    // Changes whose previous_id hasn't been seen yet; retried after each
    // successful apply.
    std::vector<ListChange> _pending_changes;

    std::string next_id();
    bool pending_contains(const std::string& operation_id) const;

    // Returns false (without throwing) if the change's causal predecessor
    // hasn't been seen yet, so the caller can buffer it.
    bool try_apply_change(const ListChange& change);

    void retry_pending_changes();
    void render_from(
        const std::string& previous_id,
        std::vector<std::string>& output
    ) const;

public:
    explicit ListRGA(std::string client_id);

    ListChange insert_at_beginning(const std::string& value);
    ListChange insert_after(
        const std::string& previous_id,
        const std::string& value
    );

    // Tombstones rather than removing: hard removal would break convergence
    // if a concurrent insert later referenced this node as previous_id.
    ListChange erase(const std::string& element_id);

    // Applies a local or remote change. Already-seen or unreplayable ops are
    // deduped or buffered; nothing fails noisily.
    void apply(const ListChange& change);

    ListRGAState state() const;

    // Tombstones win on either side, so deletes are never resurrected by an
    // older incoming state.
    void merge(const ListRGAState& other);

    std::vector<std::string> value() const;
    std::string to_string() const;
    bool has_applied(const std::string& operation_id) const;
};

#endif
