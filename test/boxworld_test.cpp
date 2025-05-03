#include <boxworld/boxworld.h>

#include <iostream>
#include <unordered_map>

using namespace boxworld;

namespace {
const std::unordered_map<std::string, int> ActionMap{
    {"w", 0}, {"d", 1}, {"s", 2}, {"a", 3}, {"e", 4},
};

void print_state(const BoxWorldGameState &state) {
    std::cout << state << std::endl;
    std::cout << state.get_hash() << std::endl;
    std::cout << std::endl;
    std::cout << "Reward signal: " << state.get_reward_signal() << std::endl;
}

void test_play() {
    std::string board_str;
    std::cout << "Enter board str: ";
    std::cin >> board_str;

    BoxWorldGameState state(board_str);
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
}    // namespace

int main() {
    test_play();
}
