#include "boxworld_base.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <exception>
#include <sstream>

namespace boxworld {

SharedStateInfo::SharedStateInfo(const GameParameters& params)
    : params(params),
      rng_seed(std::get<int>(params.at("rng_seed"))),
      game_board_str(std::get<std::string>(params.at("game_board_str"))) {}

auto SharedStateInfo::operator==(const SharedStateInfo& other) const -> bool {
    return rows == other.rows && cols == other.cols;
}

auto LocalState::operator==(const LocalState& other) const -> bool {
    return agent_idx == other.agent_idx && inventory == other.inventory && board == other.board;
}

BoxWorldGameState::BoxWorldGameState(const GameParameters& params)
    : shared_state(std::make_shared<SharedStateInfo>(params)) {
    reset();
}

auto BoxWorldGameState::operator==(const BoxWorldGameState& other) const noexcept -> bool {
    return local_state == other.local_state && *shared_state == *other.shared_state;
}

const std::vector<Action> BoxWorldGameState::ALL_ACTIONS{Action::kUp, Action::kRight, Action::kDown, Action::kLeft};

// ---------------------------------------------------------------------------

// Data
constexpr std::array kElementToStrMap{
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
};
static_assert(kElementToStrMap.size() == kNumElements);

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
const Pixel WHITE = {0xff, 0xff, 0xff};
const Pixel BLACK = {0x00, 0x00, 0x00};
const std::unordered_map<Element, Pixel> kElementToPixelMap{
    {Element::kColour0, {0x9c, 0x5a, 0x3c}},
    {Element::kColour1, {0xed, 0x1c, 0x24}},
    {Element::kColour2, {0xff, 0xa3, 0xb1}},
    {Element::kColour3, {0xff, 0x7e, 0x00}},
    {Element::kColour4, {0xe5, 0xaa, 0x7a}},
    {Element::kColour5, {0xff, 0xc2, 0x0e}},
    {Element::kColour6, {0xf5, 0xe4, 0x9c}},
    {Element::kColour7, {0xa8, 0xe6, 0x1d}},
    {Element::kColour8, {0x22, 0xb1, 0x4c}},
    {Element::kColour9, {0x00, 0xb7, 0xef}},
    {Element::kColour10, {0x6f, 0x31, 0x98}},
    {Element::kColour11, {0x2f, 0x36, 0x99}},
    {Element::kColourGoal, WHITE},
    {Element::kAgent, {0x99, 0x00, 0x30}},
    {Element::kEmpty, {0xb4, 0xb4, 0xb4}},
};

// ---------------------------------------------------------------------------

void BoxWorldGameState::reset() {
    // Board, local, and shared state info
    local_state = LocalState();

    // Parse board
    ParseBoard();
    InitKeyLockIndices();

    // zorbist hashing
    std::mt19937 gen(shared_state->rng_seed);
    std::uniform_int_distribution<uint64_t> dist(0);
    const auto channel_size = shared_state->rows * shared_state->cols;
    // Game board
    shared_state->zrbht_board.clear();
    shared_state->zrbht_board.reserve(channel_size * kNumElements);
    for (std::size_t channel = 0; channel < kNumElements; ++channel) {
        for (std::size_t i = 0; i < channel_size; ++i) {
            shared_state->zrbht_board.push_back(dist(gen));
        }
    }
    // Inventory keys
    for (std::size_t i = 0; i < kNumColours; ++i) {
        shared_state->zrbht_inventory.push_back(dist(gen));
    }

    // Set initial hash
    for (std::size_t i = 0; i < shared_state->cols * shared_state->rows; ++i) {
        local_state.zorb_hash ^=
            shared_state->zrbht_board.at((static_cast<std::size_t>(local_state.board.at(i)) * channel_size) + i);
    }
}

void BoxWorldGameState::apply_action(Action action) noexcept {
    local_state.reward_signal_colour = 0;
    local_state.reward_signal_index = 0;
    const auto agent_idx = local_state.agent_idx;
    // Do nothing if move puts agent out of bounds
    if (!InBounds(agent_idx, action)) {
        return;
    }
    // If empty, just move
    if (IsEmpty(agent_idx, action)) {
        MoveAgent(action);
    }

    const auto new_index = IndexFromAction(agent_idx, action);

    // Single key not part of a lock/box
    if (local_state.key_indices.find(new_index) != local_state.key_indices.end()) {
        local_state.reward_signal_colour = static_cast<std::size_t>(local_state.board[new_index]) + 1;
        local_state.key_indices.erase(new_index);
        AddToInventory(new_index);
        MoveAgent(action);
        local_state.reward_signal_index = local_state.agent_idx + 1;
    }

    // Lock/box pair and we have the corresponding key
    // Key is consumed, and we add the box colour to our inventory
    if (local_state.lock_indices.find(new_index) != local_state.lock_indices.end() && HasKey(new_index)) {
        local_state.lock_indices.erase(new_index);
        local_state.reward_signal_colour = static_cast<std::size_t>(local_state.board[new_index]) + 1;
        RemoveFromInventory();
        RemoveLock(new_index);
        AddToInventory(IndexFromAction(new_index, Action::kLeft));
        MoveAgent(action);
        local_state.reward_signal_index = local_state.agent_idx + 1;
    }
}

auto BoxWorldGameState::is_solution() const noexcept -> bool {
    return local_state.inventory == Element::kColourGoal;
}

auto BoxWorldGameState::legal_actions() const noexcept -> std::vector<Action> {
    return ALL_ACTIONS;
}

void BoxWorldGameState::legal_actions(std::vector<Action>& actions) const noexcept {
    actions.clear();
    for (const auto& a : ALL_ACTIONS) {
        actions.push_back(a);
    }
}

auto BoxWorldGameState::observation_shape() const noexcept -> std::array<std::size_t, 3> {
    return {kNumChannels, shared_state->cols, shared_state->rows};
}

auto BoxWorldGameState::observation_shape_environment() const noexcept -> std::array<std::size_t, 3> {
    return {kNumElements - 1, shared_state->cols, shared_state->rows};
}

auto BoxWorldGameState::get_observation() const noexcept -> std::vector<float> {
    const auto channel_length = shared_state->rows * shared_state->cols;
    std::vector<float> obs(kNumChannels * channel_length, 0);

    // Fill board (elements which are not empty)
    assert(local_state.board.size() == channel_length);
    for (std::size_t i = 0; i < channel_length; ++i) {
        const auto& el = local_state.board[i];
        if (el != Element::kEmpty) {
            obs[static_cast<std::size_t>(el) * channel_length + i] = 1;
        }
    }

    // Fill inventory
    if (local_state.inventory) {
        const auto inventory_channel = static_cast<std::size_t>(*local_state.inventory) + kNumElements - 1;
        const auto inventory_start_idx = inventory_channel * channel_length;
        std::fill_n(obs.begin() + static_cast<int>(inventory_start_idx), channel_length, 1);
    }

    return obs;
}

void BoxWorldGameState::get_observation(std::vector<float>& obs) const noexcept {
    const auto channel_length = shared_state->rows * shared_state->cols;
    const auto obs_size = kNumChannels * channel_length;
    obs.clear();
    obs.reserve(obs_size);
    std::fill_n(std::back_inserter(obs), obs_size, 0);

    // Fill board (elements which are not empty)
    assert(local_state.board.size() == channel_length);
    for (std::size_t i = 0; i < channel_length; ++i) {
        const auto& el = local_state.board[i];
        if (el != Element::kEmpty) {
            obs[static_cast<std::size_t>(el) * channel_length + i] = 1;
        }
    }

    // Fill inventory
    if (local_state.inventory) {
        const auto inventory_channel = static_cast<std::size_t>(*local_state.inventory) + kNumElements - 1;
        const auto inventory_start_idx = inventory_channel * channel_length;
        std::fill_n(obs.begin() + static_cast<int>(inventory_start_idx), channel_length, 1);
    }
}

auto BoxWorldGameState::get_observation_environment() const noexcept -> std::vector<float> {
    const auto channel_length = shared_state->rows * shared_state->cols;
    std::vector<float> obs((kNumElements - 1) * channel_length, 0);

    // Fill board (elements which are not empty)
    assert(local_state.board.size() == channel_length);
    for (std::size_t i = 0; i < channel_length; ++i) {
        const auto& el = local_state.board[i];
        if (el != Element::kEmpty) {
            obs[static_cast<std::size_t>(el) * channel_length + i] = 1;
        }
    }

    return obs;
}

void BoxWorldGameState::get_observation_environment(std::vector<float>& obs) const noexcept {
    const auto channel_length = shared_state->rows * shared_state->cols;
    const auto obs_size = (kNumElements - 1) * channel_length;
    obs.clear();
    obs.reserve(obs_size);
    std::fill_n(std::back_inserter(obs), obs_size, 0);

    // Fill board (elements which are not empty)
    assert(local_state.board.size() == channel_length);
    for (std::size_t i = 0; i < channel_length; ++i) {
        const auto& el = local_state.board[i];
        if (el != Element::kEmpty) {
            obs[static_cast<std::size_t>(el) * channel_length + i] = 1;
        }
    }
}

auto BoxWorldGameState::image_shape() const noexcept -> std::array<std::size_t, 3> {
    const auto rows = shared_state->rows + 2;
    const auto cols = shared_state->cols + 2;
    return {rows * SPRITE_HEIGHT, cols * SPRITE_WIDTH, SPRITE_CHANNELS};
}

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

auto BoxWorldGameState::to_image() const noexcept -> std::vector<uint8_t> {
    // Pad board with black border
    const auto rows = shared_state->rows + 2;
    const auto cols = shared_state->cols + 2;
    const auto channel_length = rows * cols;
    std::vector<uint8_t> img(channel_length * SPRITE_DATA_LEN, 0);

    // Top left item is the key held by the agent
    if (local_state.inventory) {
        const auto& el = *local_state.inventory;
        fill_sprite(img, 0, 0, cols, kElementToPixelMap.at(el));
    }

    // Reset of board is inside the border
    std::size_t board_idx = 0;
    for (std::size_t h = 1; h < rows - 1; ++h) {
        for (std::size_t w = 1; w < cols - 1; ++w) {
            const auto& el = local_state.board.at(board_idx);
            fill_sprite(img, h, w, cols, kElementToPixelMap.at(el));
            ++board_idx;
        }
    }
    return img;
}

auto BoxWorldGameState::get_reward_signal(bool use_colour) const noexcept -> uint64_t {
    return use_colour ? local_state.reward_signal_colour : local_state.reward_signal_index;
}

auto BoxWorldGameState::get_hash() const noexcept -> uint64_t {
    return local_state.zorb_hash;
}

auto BoxWorldGameState::get_agent_index() const noexcept -> uint64_t {
    return local_state.agent_idx;
}

auto BoxWorldGameState::get_indices(const Element& element) const noexcept -> std::vector<std::size_t> {
    std::vector<std::size_t> indices;
    for (std::size_t i = 0; i < local_state.board.size(); ++i) {
        if (local_state.board[i] == element) {
            indices.push_back(i);
        }
    }
    return indices;
}

auto BoxWorldGameState::get_target_indices() const noexcept -> std::vector<std::size_t> {
    std::vector<std::size_t> indices;
    indices.reserve(local_state.key_indices.size() + local_state.lock_indices.size());
    indices.insert(indices.end(), local_state.key_indices.begin(), local_state.key_indices.end());
    indices.insert(indices.end(), local_state.lock_indices.begin(), local_state.lock_indices.end());
    return indices;
}

auto BoxWorldGameState::get_item(std::size_t index) const noexcept -> Element {
    assert(index < shared_state->rows * shared_state->cols);
    return local_state.board[index];
}

auto BoxWorldGameState::get_item_str(std::size_t index) const noexcept -> std::string {
    assert(index < shared_state->rows * shared_state->cols);
    const auto el = local_state.board.at(index);
    return kElementToStrMap.at(static_cast<std::size_t>(el));
}

// ---------------------------------------------------------------------------

void BoxWorldGameState::ParseBoard() {
    std::stringstream board_ss(shared_state->game_board_str);
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
    const int rows = std::stoi(seglist[0]);
    const int cols = std::stoi(seglist[1]);
    if (seglist.size() != static_cast<std::size_t>(rows * cols) + 2) {
        throw std::invalid_argument("Supplied rows/cols does not match input board length.");
    }

    // Parse
    local_state.board.clear();
    shared_state->rows = rows;
    shared_state->cols = cols;
    for (std::size_t i = 2; i < seglist.size(); ++i) {
        const std::size_t el_idx = std::stoi(seglist[i]);
        if (el_idx > kNumElements) {
            std::cerr << shared_state->game_board_str << std::endl;
            std::cerr << el_idx << std::endl;
            throw std::invalid_argument("Unknown element type.");
        }
        const auto el = static_cast<Element>(el_idx);
        if (el == Element::kAgent) {
            local_state.agent_idx = i - 2;
        }
        local_state.board.push_back(el);
    }
}

void BoxWorldGameState::InitKeyLockIndices() noexcept {
    local_state.key_indices.clear();
    local_state.lock_indices.clear();
    for (std::size_t idx = 0; idx < local_state.board.size(); ++idx) {
        const bool is_colour = local_state.board[idx] != Element::kEmpty && local_state.board[idx] != Element::kAgent;
        const auto is_left_colour =
            InBounds(idx, Action::kLeft) && !IsAgent(idx, Action::kLeft) && !IsEmpty(idx, Action::kLeft);
        const auto is_right_colour =
            InBounds(idx, Action::kRight) && !IsAgent(idx, Action::kRight) && !IsEmpty(idx, Action::kRight);
        const auto is_single_key = is_colour && (!is_left_colour && !is_right_colour);
        const auto is_lock = is_colour && is_left_colour;
        assert(!(is_single_key && is_lock));
        if (is_single_key) {
            local_state.key_indices.insert(idx);
        }
        if (is_lock) {
            local_state.lock_indices.insert(idx);
        }
    }
}

auto BoxWorldGameState::GetItem(std::size_t index, Action action) const noexcept -> const Element& {
    return local_state.board[IndexFromAction(index, action)];
}

auto BoxWorldGameState::IsAgent(std::size_t index, Action action) const noexcept -> bool {
    return InBounds(index, action) && GetItem(index, action) == Element::kAgent;
}

auto BoxWorldGameState::IsEmpty(std::size_t index, Action action) const noexcept -> bool {
    return InBounds(index, action) && GetItem(index, action) == Element::kEmpty;
}

void BoxWorldGameState::MoveAgent(Action action) noexcept {
    const auto idx_old = local_state.agent_idx;
    const auto idx_new = IndexFromAction(idx_old, action);
    const auto flat_size = shared_state->rows * shared_state->cols;

    // Undo old hash
    local_state.zorb_hash ^=
        shared_state->zrbht_board.at(static_cast<std::size_t>(Element::kAgent) * flat_size + idx_old);
    local_state.zorb_hash ^=
        shared_state->zrbht_board.at(static_cast<std::size_t>(Element::kEmpty) * flat_size + idx_new);
    // Move
    local_state.agent_idx = idx_new;
    local_state.board[idx_old] = Element::kEmpty;
    local_state.board[idx_new] = Element::kAgent;
    // New hash
    local_state.zorb_hash ^=
        shared_state->zrbht_board.at(static_cast<std::size_t>(Element::kAgent) * flat_size + idx_new);
    local_state.zorb_hash ^=
        shared_state->zrbht_board.at(static_cast<std::size_t>(Element::kEmpty) * flat_size + idx_old);
}

void BoxWorldGameState::AddToInventory(std::size_t index) noexcept {
    assert(!local_state.inventory);
    const auto flat_size = shared_state->rows * shared_state->cols;
    local_state.inventory = local_state.board[index];
    local_state.zorb_hash ^=
        shared_state->zrbht_board.at(static_cast<std::size_t>(*local_state.inventory) * flat_size + index);
    local_state.zorb_hash ^= shared_state->zrbht_inventory[static_cast<std::size_t>(*local_state.inventory)];

    local_state.board[index] = Element::kEmpty;
    local_state.zorb_hash ^=
        shared_state->zrbht_board.at(static_cast<std::size_t>(Element::kEmpty) * flat_size + index);
}

void BoxWorldGameState::RemoveFromInventory() noexcept {
    assert(local_state.inventory);
    local_state.zorb_hash ^= shared_state->zrbht_inventory[static_cast<std::size_t>(*local_state.inventory)];
    local_state.inventory = std::nullopt;
}

void BoxWorldGameState::RemoveLock(std::size_t index) noexcept {
    const auto flat_size = shared_state->rows * shared_state->cols;
    local_state.zorb_hash ^=
        shared_state->zrbht_board.at(static_cast<std::size_t>(local_state.board[index]) * flat_size + index);
    local_state.board[index] = Element::kEmpty;
    local_state.zorb_hash ^=
        shared_state->zrbht_board.at(static_cast<std::size_t>(Element::kEmpty) * flat_size + index);
}

auto BoxWorldGameState::IndexFromAction(std::size_t index, Action action) const noexcept -> std::size_t {
    auto col = index % shared_state->cols;
    auto row = (index - col) / shared_state->cols;
    const auto& offsets = kActionOffsets[static_cast<std::size_t>(action)];    // NOLINT(*-bounds-constant-array-index)
    col += offsets.first;
    row += offsets.second;
    return (shared_state->cols * row) + col;
}

auto BoxWorldGameState::InBounds(std::size_t index, Action action) const noexcept -> bool {
    const auto rows = shared_state->rows;
    const auto cols = shared_state->cols;
    int col = static_cast<int>(index % cols);
    int row = static_cast<int>((index - col) / cols);
    const auto& offsets = kActionOffsets[static_cast<std::size_t>(action)];    // NOLINT(*-bounds-constant-array-index)
    col += offsets.first;
    row += offsets.second;
    return col >= 0 && col < static_cast<int>(cols) && row >= 0 && row < static_cast<int>(rows);
}

auto BoxWorldGameState::HasKey(std::size_t index) const noexcept -> bool {
    return local_state.inventory == local_state.board[index];
}

auto operator<<(std::ostream& os, const BoxWorldGameState& state) -> std::ostream& {
    const auto print_horz_boarder = [&]() {
        for (std::size_t w = 0; w < state.shared_state->cols + 2; ++w) {
            os << "-";
        }
        os << std::endl;
    };

    // Board
    print_horz_boarder();
    for (std::size_t row = 0; row < state.shared_state->rows; ++row) {
        os << "|";
        for (std::size_t col = 0; col < state.shared_state->cols; ++col) {
            const auto idx = row * state.shared_state->cols + col;
            const auto el = state.local_state.board.at(idx);
            os << kElementToStrMap.at(static_cast<std::size_t>(el));
        }
        os << "|" << std::endl;
    }
    print_horz_boarder();

    // Inventory
    os << "Inventory: "
       << ((state.local_state.inventory) ? kElementToStrMap.at(static_cast<std::size_t>(*state.local_state.inventory))
                                         : "")
       << std::endl;
    return os;
}

}    // namespace boxworld
