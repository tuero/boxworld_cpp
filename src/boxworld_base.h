#ifndef BOXWORLD_BASE_H_
#define BOXWORLD_BASE_H_

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "definitions.h"

namespace boxworld {

// Image properties
constexpr int SPRITE_WIDTH = 32;
constexpr int SPRITE_HEIGHT = 32;
constexpr int SPRITE_CHANNELS = 3;
constexpr int SPRITE_DATA_LEN_PER_ROW = SPRITE_WIDTH * SPRITE_CHANNELS;
constexpr int SPRITE_DATA_LEN = SPRITE_WIDTH * SPRITE_HEIGHT * SPRITE_CHANNELS;

class BoxWorldGameState {
public:
    // Internal use for packing/unpacking with pybind11 pickle
    struct InternalState {
        int rows = 0;
        int cols = 0;
        int agent_idx = 0;
        uint64_t zorb_hash = 0;
        uint64_t reward_signal = 0;
        int inventory;
        std::vector<int> board;
        std::unordered_set<int> key_indices;
        std::unordered_set<int> lock_indices;
    };

    BoxWorldGameState() = delete;
    BoxWorldGameState(const std::string &board_str);
    BoxWorldGameState(InternalState &&internal_state);

    bool operator==(const BoxWorldGameState &other) const noexcept;
    bool operator!=(const BoxWorldGameState &other) const noexcept;

    static inline std::string name = "boxworld";

    /**
     * Check if the given element is valid.
     * @param element Element to check
     * @return True if element is valid, false otherwise
     */
    [[nodiscard]] constexpr static auto is_valid_element(Element element) -> bool {
        return static_cast<int>(element) >= 0 && static_cast<int>(element) < static_cast<int>(kNumElements);
    }

    /**
     * Check if the given action is valid.
     * @param action Action to check
     * @return True if action is valid, false otherwise
     */
    [[nodiscard]] constexpr static auto is_valid_action(Action action) -> bool {
        return static_cast<int>(action) >= 0 && static_cast<int>(action) < static_cast<int>(kNumActions);
    }

    /**
     * Apply the action to the current state, and set the reward and signals.
     * @param action The action to apply, should be one of the legal actions
     */
    void apply_action(Action action) noexcept;

    /**
     * Get the number of possible actions
     * @return Count of possible actions
     */
    [[nodiscard]] constexpr static auto action_space_size() noexcept -> int {
        return kNumActions;
    }

    /**
     * Check if the state is in the solution state (agent inside exit).
     * @return True if terminal, false otherwise
     */
    [[nodiscard]] auto is_solution() const noexcept -> bool;

    /**
     * Get the shape the observations should be viewed as.
     * @return array indicating observation CHW
     */
    [[nodiscard]] auto observation_shape() const noexcept -> std::array<int, 3>;

    /**
     * Get a flat representation of the current state observation.
     * The observation should be viewed as the shape given by observation_shape().
     * @return vector where 1 represents object at position
     */
    [[nodiscard]] auto get_observation() const noexcept -> std::vector<float>;

    /**
     * Get the shape the image should be viewed as.
     * @return array indicating observation HWC
     */
    [[nodiscard]] auto image_shape() const noexcept -> std::array<int, 3>;

    /**
     * Get the flat (HWC) image representation of the current state
     * @return flattened byte vector represending RGB values (HWC)
     */
    [[nodiscard]] auto to_image() const noexcept -> std::vector<uint8_t>;

    /**
     * Get the current reward signal as a result of the previous action taken.
     * @return 0 if no reward, otherwise 1 + color of key/lock collected/opened
     */
    [[nodiscard]] auto get_reward_signal() const noexcept -> uint64_t;

    /**
     * Get the hash representation for the current state.
     * @return hash value
     */
    [[nodiscard]] auto get_hash() const noexcept -> uint64_t;

    /**
     * Get the agent index position, even if in exit
     * @return Agent index
     */
    [[nodiscard]] auto get_agent_index() const noexcept -> int;

    /**
     * Check if key is being held in inventory
     * @return True if holding key of any colour, false otherwise
     */
    [[nodiscard]] auto has_key() const noexcept -> bool;

    /**
     * Set the current key in the inventory
     */
    void set_key(Element element);

    friend auto operator<<(std::ostream &os, const BoxWorldGameState &state) -> std::ostream &;

    [[nodiscard]] auto pack() const -> InternalState {
        std::vector<int> _board;
        _board.reserve(board.size());
        for (const auto &el : board) {
            _board.push_back(static_cast<int>(el));
        }
        return {.rows = rows,
                .cols = cols,
                .agent_idx = agent_idx,
                .zorb_hash = zorb_hash,
                .reward_signal = reward_signal,
                .inventory = static_cast<int>(inventory),
                .board = std::move(_board),
                .key_indices = key_indices,
                .lock_indices = lock_indices};
    }

private:
    void InitKeyLockIndices() noexcept;
    [[nodiscard]] auto GetItem(int index, Action action) const noexcept -> const Element &;
    [[nodiscard]] auto IsAgent(int index, Action action) const noexcept -> bool;
    [[nodiscard]] auto IsEmpty(int index, Action action) const noexcept -> bool;
    [[nodiscard]] auto IndexFromAction(int index, Action action) const noexcept -> int;
    [[nodiscard]] auto InBounds(int index, Action action) const noexcept -> bool;
    void MoveAgent(Action action) noexcept;
    [[nodiscard]] auto HasKey(int index) const noexcept -> bool;
    void AddToInventory(int index) noexcept;
    void RemoveFromInventory() noexcept;
    void RemoveLock(int index) noexcept;

    int rows = 0;                            // Rows of the common board
    int cols = 0;                            // Cols of the common board
    int agent_idx = 0;                       // Board index the agent resides
    uint64_t zorb_hash = 0;                  // hash value of the current state
    uint64_t reward_signal = 0;              // Signal for external information about events
    Element inventory = Element::kAgent;     // Current key in the inventory
    std::vector<Element> board{};            // Main storage of the board
    std::unordered_set<int> key_indices;     // Fast lookup of keys
    std::unordered_set<int> lock_indices;    // Fast lookup of locks
};

}    // namespace boxworld

#endif    // BOXWORLD_BASE_H_
