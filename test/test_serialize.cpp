#include <boxworld/boxworld.h>

using namespace boxworld;

void test_serialization() {
    const std::string board_str =
        "16|16|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|11|05|14|14|14|14|14|14|14|14|14|14|14|14|14|14|02|10|"
        "14|14|14|14|14|14|14|04|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|"
        "14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|13|14|14|14|14|14|14|14|14|14|14|14|05|10|14|14|14|"
        "14|14|12|11|14|14|14|14|14|14|14|14|14|14|14|14|01|00|14|14|14|14|14|14|14|14|14|14|14|14|14|14|08|02|14|14|"
        "14|14|02|01|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|"
        "14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|01|"
        "04|14|14|14|14|14|14|14|14|14|14|14|14|14|08|01|14|10|04|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|14|"
        "14|14|14|14|14|14";
    GameParameters params = kDefaultGameParams;
    params["game_board_str"] = GameParameter(board_str);

    BoxWorldGameState state(params);
    state.apply_action(Action(1));

    std::vector<uint8_t> bytes = state.serialize();

    state.apply_action(Action(1));
    state.apply_action(Action(2));

    BoxWorldGameState state_copy(bytes);
    state_copy.apply_action(Action(1));
    state_copy.apply_action(Action(2));

    if (state != state_copy) {
        std::cout << "serialization error." << std::endl;
    }
    std::cout << state << std::endl;
    std::cout << state.get_hash() << std::endl;

    if (state.get_hash() != state_copy.get_hash()) {
        std::cout << "serialization error." << std::endl;
    }
    std::cout << state_copy << std::endl;
    std::cout << state_copy.get_hash() << std::endl;
}

int main() {
    test_serialization();
}
