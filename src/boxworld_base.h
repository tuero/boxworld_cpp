#ifndef BOXWORLD_BASE_H_
#define BOXWORLD_BASE_H_

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "definitions.h"

namespace boxworld {

// Image properties
constexpr int SPRITE_WIDTH = 32;
constexpr int SPRITE_HEIGHT = 32;
constexpr int SPRITE_CHANNELS = 3;
constexpr int SPRITE_DATA_LEN_PER_ROW = SPRITE_WIDTH * SPRITE_CHANNELS;
constexpr int SPRITE_DATA_LEN = SPRITE_WIDTH * SPRITE_HEIGHT * SPRITE_CHANNELS;

// Game parameter can be boolean, integral or floating point
using GameParameter = std::variant<bool, int, float, std::string>;
using GameParameters = std::unordered_map<std::string, GameParameter>;

// Default game parameters
static const GameParameters kDefaultGameParams{
    {"rng_seed", GameParameter(0)},    // Seed for anything that uses the rng
    {"game_board_str", GameParameter(std::string("3|3|00|20|19|20|20|20|18|00|20"))},    // Game board string
};

// Shared global state information relevant to all states for the given game
struct SharedStateInfo {
    SharedStateInfo(const GameParameters &params);
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    GameParameters params;                    // Copy of game parameters for state resetting
    int rng_seed;                             // Seed
    std::string game_board_str;               // String representation of the starting state
    std::vector<uint64_t> zrbht_board;        // Zobrist hashing table for board items
    std::vector<uint64_t> zrbht_inventory;    // Zobrist hashing table for inventory
    std::size_t rows = 0;                     // Rows of the common board
    std::size_t cols = 0;                     // Cols of the common board
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    auto operator==(const SharedStateInfo &other) const -> bool;
};

// Information specific for the current game state
struct LocalState {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    uint64_t zorb_hash = 0;                             // hash value of the current state
    uint64_t reward_signal_index = 0;                   // Signal for external information about events
    uint64_t reward_signal_colour = 0;                  // Signal for external information about events
    std::size_t agent_idx = 0;                          // Board index the agent resides
    std::vector<Element> board{};                       // Main storage of the board
    std::optional<Element> inventory = std::nullopt;    // Current key in the inventory
    std::unordered_set<std::size_t> key_indices;        // Fast lookup of keys
    std::unordered_set<std::size_t> lock_indices;       // Fast lookup of locks
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    auto operator==(const LocalState &other) const -> bool;
};

class BoxWorldGameState {
public:
    BoxWorldGameState() = delete;
    BoxWorldGameState(const GameParameters &params);

    bool operator==(const BoxWorldGameState &other) const noexcept;

    /**
     * Reset the environment to the state as given by the GameParameters
     */
    void reset();

    /**
     * Apply the action to the current state, and set the reward and signals.
     * @param action The action to apply, should be one of the legal actions
     */
    void apply_action(Action action) noexcept;

    /**
     * Check if the state is in the solution state (agent inside exit).
     * @return True if terminal, false otherwise
     */
    [[nodiscard]] auto is_solution() const noexcept -> bool;

    /**
     * Get the legal actions which can be applied in the state.
     * @return vector containing each actions available
     */
    [[nodiscard]] auto legal_actions() const noexcept -> const std::vector<Action> &;

    /**
     * Get the number of possible actions
     * @return Count of possible actions
     */
    [[nodiscard]] constexpr static auto action_space_size() noexcept -> std::size_t {
        return kNumActions;
    }

    /**
     * Get the shape the observations should be viewed as.
     * @return array indicating observation CHW
     */
    [[nodiscard]] auto observation_shape() const noexcept -> std::array<std::size_t, 3>;

    /**
     * Get the shape the observations without the inventory should be viewed as.
     * @return array indicating observation CHW
     */
    [[nodiscard]] auto observation_shape_environment() const noexcept -> std::array<std::size_t, 3>;

    /**
     * Get a flat representation of the current state observation.
     * The observation should be viewed as the shape given by observation_shape().
     * @return vector where 1 represents object at position
     */
    [[nodiscard]] auto get_observation() const noexcept -> std::vector<float>;

    /**
     * Get a flat representation of the current state observation.
     * The observation should be viewed as the shape given by observation_shape().
     * @return vector where 1 represents object at position
     */
    [[nodiscard]] auto get_observation_environment() const noexcept -> std::vector<float>;

    /**
     * Get the shape the image should be viewed as.
     * @return array indicating observation HWC
     */
    [[nodiscard]] auto image_shape() const noexcept -> std::array<std::size_t, 3>;

    /**
     * Get the flat (HWC) image representation of the current state
     * @return flattened byte vector represending RGB values (HWC)
     */
    [[nodiscard]] auto to_image() const noexcept -> std::vector<uint8_t>;

    /**
     * Get the current reward signal as a result of the previous action taken.
     * @param use_colour Flag if using colour collected signal, or index of key/lock collected if false
     * @return 0 if no reward, otherwise 1 + index of key/lock collected/opened
     */
    [[nodiscard]] auto get_reward_signal(bool use_colour = false) const noexcept -> uint64_t;

    /**
     * Get the hash representation for the current state.
     * @return hash value
     */
    [[nodiscard]] auto get_hash() const noexcept -> uint64_t;

    /**
     * Get the agent index position, even if in exit
     * @return Agent index
     */
    [[nodiscard]] auto get_agent_index() const noexcept -> std::size_t;

    /**
     * Get all indices for a given element type
     * @param element The hidden cell type of the element to search for
     * @return flat indices for each instance of element
     */
    [[nodiscard]] auto get_indices(const Element &element) const noexcept -> std::vector<std::size_t>;

    /**
     * Get element at given index
     * @param index Index to query
     * @return Element at given index
     */
    [[nodiscard]] auto get_item(std::size_t index) const noexcept -> Element;

    /**
     * Get string representation of element at given index
     * @param index Index to query
     * @return string representation of element
     */
    [[nodiscard]] auto get_item_str(std::size_t index) const noexcept -> std::string;

    /**
     * Get all indices for target elements of the agent (single keys + locks)
     * @return flat indices for each single key + lock
     */
    [[nodiscard]] auto get_target_indices() const noexcept -> std::vector<std::size_t>;

    friend auto operator<<(std::ostream &os, const BoxWorldGameState &state) -> std::ostream &;

private:
    void ParseBoard();
    void InitKeyLockIndices() noexcept;
    [[nodiscard]] auto GetItem(std::size_t index, Action action) const noexcept -> const Element &;
    [[nodiscard]] auto IsAgent(std::size_t index, Action action) const noexcept -> bool;
    [[nodiscard]] auto IsEmpty(std::size_t index, Action action) const noexcept -> bool;
    [[nodiscard]] auto IndexFromAction(std::size_t index, Action action) const noexcept -> std::size_t;
    [[nodiscard]] auto InBounds(std::size_t index, Action action) const noexcept -> bool;
    void MoveAgent(Action action) noexcept;
    [[nodiscard]] auto HasKey(std::size_t index) const noexcept -> bool;
    void AddToInventory(std::size_t index) noexcept;
    void RemoveFromInventory() noexcept;
    void RemoveLock(std::size_t index) noexcept;

    std::shared_ptr<SharedStateInfo> shared_state;
    LocalState local_state;
};

}    // namespace boxworld

#endif    // BOXWORLD_BASE_H_