#include <boxworld/boxworld.h>

#include <iostream>

using namespace boxworld;

const std::unordered_map<std::string, int> ActionMap{
    {"w", 0}, {"d", 1}, {"s", 2}, {"a", 3}, {"e", 4},
};

void print_state(const BoxWorldGameState &state) {
    std::cout << state << std::endl;
    std::cout << state.get_hash() << std::endl;
    std::cout << "Target indices: ";
    for (auto const &i : state.get_target_indices()) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    std::cout << "Reward signal: " << state.get_reward_signal(true) << std::endl;
    std::cout << "Reward signal: " << state.get_reward_signal(false) << std::endl;
}

void test_play() {
    std::string board_str;
    std::cout << "Enter board str: ";
    std::cin >> board_str;
    GameParameters params = kDefaultGameParams;
    params["game_board_str"] = GameParameter(board_str);

    BoxWorldGameState state(params);
    print_state(state);

    std::string action_str;
    while (!state.is_solution()) {
        std::cin >> action_str;
        if (ActionMap.find(action_str) != ActionMap.end()) {
            state.apply_action(static_cast<Action>(ActionMap.at(action_str)));
        }
        print_state(state);
    }
}

int main() {
    test_play();
}
