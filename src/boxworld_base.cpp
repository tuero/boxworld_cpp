#include "boxworld_base.h"

#include <array>
#include <cassert>
#include <sstream>
#include <unordered_map>

namespace boxworld {

namespace {
// Data
const std::array<std::string, kNumElements> kElementToStrMap{
    "a",    // kColour0
    "b",    // kColour1
    "c",    // kColour2
    "d",    // kColour3
    "e",    // kColour4
    "f",    // kColour5
    "g",    // kColour6
    "h",    // kColour7
    "i",    // kColour8
    "j",    // kColour9
    "k",    // kColour10
    "l",    // kColour11
    "!",    // kColourGoal
    "@",    // kAgent
    " ",    // kEmpty
    "#",    // kWall
};

// Direction to offsets (col, row)
using Offset = std::pair<int, int>;
constexpr std::array<Offset, kNumActions> kActionOffsets{{
    {0, -1},    // Action::kUp
    {1, 0},     // Action::kRight
    {0, 1},     // Action::kDown
    {-1, 0},    // Action::kLeft
}};
static_assert(kActionOffsets.size() == kNumActions);

// Colour maps for state to image
struct Pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};
const Pixel WHITE = {.r = 0xff, .g = 0xff, .b = 0xff};
const Pixel BLACK = {.r = 0x00, .g = 0x00, .b = 0x00};
const std::unordered_map<Element, Pixel> kElementToPixelMap{
    {Element::kColour0, {.r = 0xfe, .g = 0x00, .b = 0x00}},     // light red
    {Element::kColour1, {.r = 0x80, .g = 0x00, .b = 0x01}},     // dark red
    {Element::kColour2, {.r = 0xff, .g = 0xb7, .b = 0x32}},     // orange
    {Element::kColour3, {.r = 0x80, .g = 0x34, .b = 0x00}},     // brown
    {Element::kColour4, {.r = 0xff, .g = 0xff, .b = 0x00}},     // yellow
    {Element::kColour5, {.r = 0x00, .g = 0xfe, .b = 0x21}},     // light green
    {Element::kColour6, {.r = 0x00, .g = 0x7f, .b = 0x0e}},     // dark green
    {Element::kColour7, {.r = 0x32, .g = 0xa9, .b = 0xfe}},     // light blue
    {Element::kColour8, {.r = 0x00, .g = 0x26, .b = 0xff}},     // blue
    {Element::kColour9, {.r = 0x00, .g = 0xe6, .b = 0x66}},     // dark blue
    {Element::kColour10, {.r = 0xb1, .g = 0x00, .b = 0xfe}},    // light purple
    {Element::kColour11, {.r = 0x47, .g = 0x00, .b = 0x66}},    // dark purple
    {Element::kColourGoal, WHITE},
    {Element::kAgent, BLACK},
    {Element::kEmpty, {.r = 0xb4, .g = 0xb4, .b = 0xb4}},
};

constexpr uint64_t SPLIT64_S1 = 30;
constexpr uint64_t SPLIT64_S2 = 27;
constexpr uint64_t SPLIT64_S3 = 31;
constexpr uint64_t SPLIT64_C1 = 0x9E3779B97f4A7C15;
constexpr uint64_t SPLIT64_C2 = 0xBF58476D1CE4E5B9;
constexpr uint64_t SPLIT64_C3 = 0x94D049BB133111EB;

template <class E>
constexpr inline auto to_underlying(E e) noexcept -> std::underlying_type_t<E> {
    return static_cast<std::underlying_type_t<E>>(e);
}

auto to_local_hash(int flat_size, Element el, int offset) noexcept -> uint64_t {
    uint64_t seed = (flat_size * to_underlying(el)) + offset;
    uint64_t result = seed + SPLIT64_C1;
    result = (result ^ (result >> SPLIT64_S1)) * SPLIT64_C2;
    result = (result ^ (result >> SPLIT64_S2)) * SPLIT64_C3;
    return result ^ (result >> SPLIT64_S3);
}

auto to_local_hash_inventory(int flat_size, Element el) noexcept -> uint64_t {
    uint64_t seed = (flat_size * to_underlying(el)) + 0xFFFFFFFF;    // NOLINT(*-magic-numbers)
    uint64_t result = seed + SPLIT64_C1;
    result = (result ^ (result >> SPLIT64_S1)) * SPLIT64_C2;
    result = (result ^ (result >> SPLIT64_S2)) * SPLIT64_C3;
    return result ^ (result >> SPLIT64_S3);
}

}    // namespace

BoxWorldGameState::BoxWorldGameState(const std::string& board_str) {
    std::stringstream board_ss(board_str);
    std::string segment;
    std::vector<std::string> seglist;
    // string split on |
    while (std::getline(board_ss, segment, '|')) {
        seglist.push_back(segment);
    }

    // Check input
    if (seglist.size() < 2) {
        throw std::invalid_argument("Board string should have at minimum 3 values separated by '|'.");
    }
    rows = std::stoi(seglist[0]);
    cols = std::stoi(seglist[1]);
    if (seglist.size() != static_cast<std::size_t>(rows * cols) + 2) {
        throw std::invalid_argument("Supplied rows/cols does not match input board length.");
    }

    // Parse
    for (std::size_t i = 2; i < seglist.size(); ++i) {
        const auto el_idx = static_cast<std::size_t>(std::stoi(seglist[i]));
        if (el_idx > kNumElements) {
            std::cerr << el_idx << std::endl;
            throw std::invalid_argument("Unknown element type.");
        }
        const auto el = static_cast<Element>(el_idx);
        if (el == Element::kAgent) {
            agent_idx = static_cast<int>(i) - 2;
        }
        board.push_back(el);
    }

    InitKeyLockIndices();

    // Set initial hash
    const auto flat_size = rows * cols;
    for (int i = 0; i < flat_size; ++i) {
        zorb_hash ^= to_local_hash(flat_size, board[static_cast<std::size_t>(i)], i);
    }

    zorb_hash ^= to_local_hash_inventory(flat_size, inventory);
}

BoxWorldGameState::BoxWorldGameState(InternalState&& internal_state)
    : rows(internal_state.rows),
      cols(internal_state.cols),
      agent_idx(internal_state.agent_idx),
      zorb_hash(internal_state.zorb_hash),
      reward_signal(internal_state.reward_signal),
      inventory(static_cast<Element>(internal_state.inventory)),
      key_indices(std::move(internal_state).key_indices),
      lock_indices(std::move(internal_state).key_indices) {
    board.clear();
    for (const auto& b : internal_state.board) {
        board.push_back(static_cast<Element>(b));
    }
}

auto BoxWorldGameState::operator==(const BoxWorldGameState& other) const noexcept -> bool {
    return rows == other.rows && cols == other.cols && agent_idx == other.agent_idx && inventory == other.inventory &&
           board == other.board && key_indices == other.key_indices && lock_indices == other.lock_indices;
}

auto BoxWorldGameState::operator!=(const BoxWorldGameState& other) const noexcept -> bool {
    return !(*this == other);
}

// ---------------------------------------------------------------------------

void BoxWorldGameState::apply_action(Action action) noexcept {
    assert(is_valid_action(action));

    reward_signal = 0;
    // Do nothing if move puts agent out of bounds
    if (!InBounds(agent_idx, action)) {
        return;
    }
    // If empty, just move
    if (IsEmpty(agent_idx, action)) {
        MoveAgent(action);
        return;
    }

    const auto new_index = IndexFromAction(agent_idx, action);

    // Single key not part of a lock/box
    if (key_indices.find(new_index) != key_indices.end()) {
        reward_signal = static_cast<int>(board[new_index]) + 1;
        key_indices.erase(new_index);
        AddToInventory(new_index);
        MoveAgent(action);
    }

    // Lock/box pair and we have the corresponding key
    // Key is consumed, and we add the box colour to our inventory
    if (lock_indices.find(new_index) != lock_indices.end() && HasKey(new_index)) {
        lock_indices.erase(new_index);
        reward_signal = static_cast<int>(board[new_index]) + 1;
        RemoveFromInventory();
        RemoveLock(new_index);
        AddToInventory(IndexFromAction(new_index, Action::kLeft));
        MoveAgent(action);
    }
}

auto BoxWorldGameState::is_solution() const noexcept -> bool {
    return inventory == Element::kColourGoal;
}

auto BoxWorldGameState::observation_shape() const noexcept -> std::array<int, 3> {
    return {kNumChannels, cols + 2, rows + 2};
}

auto BoxWorldGameState::get_observation() const noexcept -> std::vector<float> {
    int rows_obs = rows + 2;
    int cols_obs = cols + 2;
    auto channel_length = rows_obs * cols_obs;
    std::vector<float> obs(kNumChannels * channel_length, 0);

    // outer wall
    for (int w = 1; w < cols_obs - 1; ++w) {
        const auto channel = static_cast<std::size_t>(Element::kWall);
        obs[channel * channel_length + (0 * cols_obs + w)] = 1;
        obs[channel * channel_length + ((rows_obs - 1) * cols_obs + w)] = 1;
    }
    for (int h = 1; h < rows_obs - 1; ++h) {
        const auto channel = static_cast<std::size_t>(Element::kWall);
        obs[channel * channel_length + (h * cols_obs + 0)] = 1;
        obs[channel * channel_length + (h * cols_obs + (cols_obs - 1))] = 1;
    }

    int i = -1;
    for (int r = 1; r < rows_obs - 1; ++r) {
        for (int c = 1; c < cols_obs - 1; ++c) {
            auto el = board[static_cast<std::size_t>(++i)];
            auto obs_idx = (r * cols_obs) + c;
            obs[static_cast<std::size_t>(el) * channel_length + obs_idx] = 1;
        }
    }

    // Key top left
    if (has_key()) {
        const auto inventory_channel = static_cast<int>(inventory);
        obs[inventory_channel * channel_length + 0] = 1;
    }

    return obs;
}

auto BoxWorldGameState::image_shape() const noexcept -> std::array<int, 3> {
    const auto r = rows + 2;
    const auto c = cols + 2;
    return {r * SPRITE_HEIGHT, c * SPRITE_WIDTH, SPRITE_CHANNELS};
}

namespace {
void fill_sprite(std::vector<uint8_t>& img, std::size_t h, std::size_t w, std::size_t cols, const Pixel& pixel) {
    const std::size_t img_idx_top_left = h * (SPRITE_DATA_LEN * cols) + (w * SPRITE_DATA_LEN_PER_ROW);
    for (std::size_t r = 0; r < SPRITE_HEIGHT; ++r) {
        for (std::size_t c = 0; c < SPRITE_WIDTH; ++c) {
            const std::size_t img_idx = (r * SPRITE_DATA_LEN_PER_ROW * cols) + (SPRITE_CHANNELS * c) + img_idx_top_left;
            img[img_idx + 0] = pixel.r;
            img[img_idx + 1] = pixel.g;
            img[img_idx + 2] = pixel.b;
        }
    }
}
}    // namespace

auto BoxWorldGameState::to_image() const noexcept -> std::vector<uint8_t> {
    // Pad board with black border
    const auto rows_obs = rows + 2;
    const auto cols_obs = cols + 2;
    const auto channel_length = rows_obs * cols_obs;
    std::vector<uint8_t> img(channel_length * SPRITE_DATA_LEN, 0);

    // Top left item is the key held by the agent
    if (has_key()) {
        fill_sprite(img, 0, 0, cols_obs, kElementToPixelMap.at(inventory));
    }

    // Reset of board is inside the border
    int board_idx = -1;
    for (int h = 1; h < rows_obs - 1; ++h) {
        for (int w = 1; w < cols_obs - 1; ++w) {
            const auto& el = board[static_cast<std::size_t>(++board_idx)];
            fill_sprite(img, h, w, cols, kElementToPixelMap.at(el));
        }
    }
    return img;
}

auto BoxWorldGameState::get_reward_signal() const noexcept -> uint64_t {
    return reward_signal;
}

auto BoxWorldGameState::get_hash() const noexcept -> uint64_t {
    return zorb_hash;
}

auto BoxWorldGameState::get_agent_index() const noexcept -> int {
    return agent_idx;
}

auto BoxWorldGameState::has_key() const noexcept -> bool {
    return inventory != Element::kAgent;
}

void BoxWorldGameState::set_key(Element element) {
    if (!is_valid_element(element) || element == Element::kEmpty || element == Element::kAgent ||
        element == Element::kWall) {
        throw std::invalid_argument("Unknown key element.");
    }
    if (has_key()) {
        throw std::invalid_argument("Already has key.");
    }
    if (!key_indices.empty()) {
        throw std::invalid_argument("Single key already exists.");
    }
    // Remove current key
    const auto flat_size = rows * cols;
    zorb_hash ^= to_local_hash_inventory(flat_size, inventory);
    // Set key
    inventory = element;
    zorb_hash ^= to_local_hash_inventory(flat_size, inventory);
}

// ---------------------------------------------------------------------------

void BoxWorldGameState::InitKeyLockIndices() noexcept {
    key_indices.clear();
    lock_indices.clear();
    for (int idx = 0; idx < rows * cols; ++idx) {
        const bool is_colour = board[idx] != Element::kEmpty && board[idx] != Element::kAgent;
        const auto is_left_colour =
            InBounds(idx, Action::kLeft) && !IsAgent(idx, Action::kLeft) && !IsEmpty(idx, Action::kLeft);
        const auto is_right_colour =
            InBounds(idx, Action::kRight) && !IsAgent(idx, Action::kRight) && !IsEmpty(idx, Action::kRight);
        const auto is_single_key = is_colour && (!is_left_colour && !is_right_colour);
        const auto is_lock = is_colour && is_left_colour;
        assert(!(is_single_key && is_lock));
        if (is_single_key) {
            key_indices.insert(idx);
        }
        if (is_lock) {
            lock_indices.insert(idx);
        }
    }
}

auto BoxWorldGameState::GetItem(int index, Action action) const noexcept -> const Element& {
    return board[static_cast<std::size_t>(IndexFromAction(index, action))];
}

auto BoxWorldGameState::IsAgent(int index, Action action) const noexcept -> bool {
    return InBounds(index, action) && GetItem(index, action) == Element::kAgent;
}

auto BoxWorldGameState::IsEmpty(int index, Action action) const noexcept -> bool {
    return InBounds(index, action) && GetItem(index, action) == Element::kEmpty;
}

void BoxWorldGameState::MoveAgent(Action action) noexcept {
    const auto idx_old = agent_idx;
    const auto idx_new = IndexFromAction(idx_old, action);
    const auto flat_size = rows * cols;

    // Undo old hash
    zorb_hash ^= to_local_hash(flat_size, Element::kAgent, idx_old);
    zorb_hash ^= to_local_hash(flat_size, Element::kEmpty, idx_new);
    // Move
    agent_idx = idx_new;
    board[static_cast<std::size_t>(idx_old)] = Element::kEmpty;
    board[static_cast<std::size_t>(idx_new)] = Element::kAgent;
    // New hash
    zorb_hash ^= to_local_hash(flat_size, Element::kAgent, idx_new);
    zorb_hash ^= to_local_hash(flat_size, Element::kEmpty, idx_old);
}

void BoxWorldGameState::AddToInventory(int index) noexcept {
    assert(!has_key());
    const auto flat_size = rows * cols;

    // Add item to inventory
    zorb_hash ^= to_local_hash_inventory(flat_size, inventory);
    inventory = board[static_cast<std::size_t>(index)];
    zorb_hash ^= to_local_hash_inventory(flat_size, inventory);

    // Remove item from board
    zorb_hash ^= to_local_hash(flat_size, board[static_cast<std::size_t>(index)], index);
    board[static_cast<std::size_t>(index)] = Element::kEmpty;
    zorb_hash ^= to_local_hash(flat_size, Element::kEmpty, index);
}

void BoxWorldGameState::RemoveFromInventory() noexcept {
    assert(has_key());
    const auto flat_size = rows * cols;
    zorb_hash ^= to_local_hash_inventory(flat_size, inventory);
    inventory = Element::kAgent;
    zorb_hash ^= to_local_hash_inventory(flat_size, inventory);
}

void BoxWorldGameState::RemoveLock(int index) noexcept {
    const auto flat_size = rows * cols;
    zorb_hash ^= to_local_hash(flat_size, board[static_cast<std::size_t>(index)], index);
    board[index] = Element::kEmpty;
    zorb_hash ^= to_local_hash(flat_size, Element::kEmpty, index);
}

auto BoxWorldGameState::IndexFromAction(int index, Action action) const noexcept -> int {
    auto col = index % cols;
    auto row = (index - col) / cols;
    const auto& offsets = kActionOffsets[static_cast<std::size_t>(action)];    // NOLINT(*-bounds-constant-array-index)
    col += offsets.first;
    row += offsets.second;
    return (cols * row) + col;
}

auto BoxWorldGameState::InBounds(int index, Action action) const noexcept -> bool {
    auto col = static_cast<int>(index) % cols;
    auto row = (static_cast<int>(index) - col) / cols;
    const auto& offsets = kActionOffsets[static_cast<std::size_t>(action)];    // NOLINT(*-bounds-constant-array-index)
    col = col + offsets.first;
    row = row + offsets.second;
    return col >= 0 && col < cols && row >= 0 && row < rows;
}

auto BoxWorldGameState::HasKey(int index) const noexcept -> bool {
    return inventory == board[static_cast<std::size_t>(index)];
}

auto operator<<(std::ostream& os, const BoxWorldGameState& state) -> std::ostream& {
    const auto print_horz_boarder = [&]() {
        for (int w = 0; w < state.cols + 2; ++w) {
            os << "-";
        }
        os << std::endl;
    };

    // Board
    print_horz_boarder();
    for (int row = 0; row < state.rows; ++row) {
        os << "|";
        for (int col = 0; col < state.cols; ++col) {
            const auto idx = row * state.cols + col;
            const auto el = state.board.at(idx);
            os << kElementToStrMap.at(static_cast<std::size_t>(el));
        }
        os << "|" << std::endl;
    }
    print_horz_boarder();

    // Inventory
    os << "Inventory: " << (state.has_key() ? kElementToStrMap.at(static_cast<std::size_t>(state.inventory)) : "")
       << std::endl;
    return os;
}

}    // namespace boxworld
