#ifndef BOXWORLD_DEFS_H_
#define BOXWORLD_DEFS_H_

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
    kWall,
};

constexpr int kNumElements = 16;
constexpr int kNumColours = 13;
constexpr int kNumChannels = kNumElements;

// Possible actions for the agent to take
enum class Action {
    kUp = 0,
    kRight = 1,
    kDown = 2,
    kLeft = 3,
};
constexpr int kNumActions = 4;

}    // namespace boxworld

#endif    // BOXWORLD_DEFS_H_
