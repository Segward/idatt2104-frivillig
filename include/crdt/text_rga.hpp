#ifndef TEXT_RGA_HPP
#define TEXT_RGA_HPP

#include <crdt/text.hpp>

// Replicated Growable Array (RGA) for collaborative plain text. Lets multiple
// clients insert and delete characters concurrently and still converge on
// the same document, without a central lock. Each insert is causally chained
// to its predecessor; operations that arrive before their dependency are
// buffered and retried once it lands.
class TextRGA {
private:
    std::string _client_id;
    std::uint64_t _local_sequence = 0;

    std::unordered_map<std::string, TextCharacter> _nodes;
    std::unordered_map<std::string, std::vector<std::string>> _children;
    std::unordered_set<std::string> _applied_operations;

    // Changes whose previous_id hasn't been seen yet; retried after each
    // successful apply.
    std::vector<TextChange> _pending_changes;

    std::string next_id();
    bool node_exists(const std::string& node_id) const;
    bool pending_contains(const std::string& operation_id) const;

    // Returns false (without throwing) if the change's causal predecessor
    // hasn't been seen yet, so the caller can buffer it.
    bool try_apply_change(const TextChange& change);

    void retry_pending_changes();
    void render_from(
        const std::string& previous_id,
        std::string& output
    ) const;

public:
    explicit TextRGA(std::string client_id);

    TextChange insert_at_beginning(std::string value);
    TextChange insert_after(
        const std::string& previous_id,
        std::string value
    );

    // Tombstones rather than removing: hard removal would break convergence
    // if a concurrent insert later referenced this character as previous_id.
    TextChange erase(const std::string& element_id);

    // Applies a local or remote change. Already-seen or unreplayable ops are
    // deduped or buffered; nothing fails noisily.
    void apply(const TextChange& change);

    TextRGAState state() const;

    // Tombstones win on either side, so deletes are never resurrected by an
    // older incoming state.
    void merge(const TextRGAState& other);

    std::string value() const;
    bool has_applied(const std::string& operation_id) const;
};

#endif
