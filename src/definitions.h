#ifndef BOXWORLD_DEFS_H_
#define BOXWORLD_DEFS_H_

#include <cstdint>

namespace boxworld {

enum class Element {
    kColour0 = 0,
    kColour1,
    kColour2,
    kColour3,
    kColour4,
    kColour5,
    kColour6,
    kColour7,
    kColour8,
    kColour9,
    kColour10,
    kColour11,
    kColourGoal,
    kAgent,
    kEmpty,
};

constexpr std::size_t kNumElements = 15;
constexpr std::size_t kNumColours = 13;
constexpr std::size_t kNumChannels = kNumElements - 1 + kNumColours;    // Board elements (not empty) + inventory key

// Possible actions for the agent to take
enum class Action {
    kUp = 0,
    kRight = 1,
    kDown = 2,
    kLeft = 3,
};
constexpr std::size_t kNumActions = 4;

}    // namespace boxworld

#endif    // BOXWORLD_DEFS_H_
