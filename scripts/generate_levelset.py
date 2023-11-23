# Source from: https://github.com/nathangrinsztajn/Box-World/

import argparse
import os
from multiprocessing import Manager, Pool

import numpy as np


def sample_from_set(s, rng):
    return list(s)[rng.choice(len(s))]


def sampling_pairs(num_pair, n, rng):
    possibilities = set(range(1, n * (n - 1)))
    keys = []
    locks = []
    for _ in range(num_pair):
        key = sample_from_set(possibilities, rng)
        key_x, key_y = key // (n - 1), key % (n - 1)
        lock_x, lock_y = key_x, key_y + 1
        to_remove = (
            [key_x * (n - 1) + key_y]
            + [key_x * (n - 1) + i + key_y for i in range(1, min(2, n - 2 - key_y) + 1)]
            + [key_x * (n - 1) - i + key_y for i in range(1, min(2, key_y) + 1)]
        )

        possibilities -= set(to_remove)
        keys.append([key_x, key_y])
        locks.append([lock_x, lock_y])
    agent_pos = [sample_from_set(possibilities, rng)]
    possibilities -= set(agent_pos)
    first_key = [sample_from_set(possibilities, rng)]

    agent_pos = np.array([agent_pos[0] // (n - 1), agent_pos[0] % (n - 1)])
    first_key = first_key[0] // (n - 1), first_key[0] % (n - 1)
    return keys, locks, first_key, agent_pos


kNumColors = 12
kGoal = 12
kAgent = 13
kEmpty = 14


num_colors = kNumColors


def create_map(args):
    manager_dict, n, goal_length, num_distractor, distractor_length, seed = args
    rng = np.random.default_rng(seed)

    map = np.ones((n, n)) * kEmpty
    goal_colors = rng.choice(
        range(num_colors), size=goal_length - 1, replace=False
    ).tolist()
    distractor_possible_colors = [
        color for color in range(num_colors) if color not in goal_colors
    ]
    distractor_colors = [
        rng.choice(
            distractor_possible_colors, size=distractor_length, replace=False
        ).tolist()
        for _ in range(num_distractor)
    ]
    distractor_roots = rng.choice(
        range(goal_length - 1), size=num_distractor, replace=True
    )
    keys, locks, first_key, agent_pos = sampling_pairs(
        goal_length - 1 + distractor_length * num_distractor, n, rng
    )

    # first, create the goal path
    for i in range(1, goal_length):
        if i == goal_length - 1:
            c = kGoal
        else:
            c = goal_colors[i]
        map[keys[i - 1][0], keys[i - 1][1]] = c
        map[locks[i - 1][0], locks[i - 1][1]] = goal_colors[i - 1]

    # keys[0] is an orphand key so skip it
    map[first_key[0], first_key[1]] = goal_colors[0]

    # place distractors
    for i, (distractor_color, root) in enumerate(
        zip(distractor_colors, distractor_roots)
    ):
        key_distractor = keys[
            goal_length
            - 1
            + i * distractor_length : goal_length
            - 1
            + (i + 1) * distractor_length
        ]
        c_lock = goal_colors[root]
        c_key = distractor_color[0]
        map[key_distractor[0][0], key_distractor[0][1] + 1] = c_lock
        map[key_distractor[0][0], key_distractor[0][1]] = c_key
        for k, key in enumerate(key_distractor[1:]):
            c_lock = distractor_color[k]
            c_key = distractor_color[k - 1]
            map[key[0], key[1]] = c_key
            map[key[0], key[1] + 1] = c_lock

    # place an agent
    map[agent_pos[0], agent_pos[1]] = kAgent
    map = map.flatten().astype(int).tolist()
    map = [n, n] + map
    map = "|".join("{:0>2}".format(x) for x in map)
    manager_dict[seed] = map


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--num_train",
        help="Number of maps in train set",
        required=False,
        type=int,
        default=10000,
    )
    parser.add_argument(
        "--num_test",
        help="Number of maps in test set",
        required=False,
        type=int,
        default=1000,
    )
    parser.add_argument(
        "--map_size",
        help="Size of map width/height",
        required=False,
        type=int,
        default=10,
    )
    parser.add_argument(
        "--goal_length",
        help="Length of the goal paths",
        required=False,
        type=int,
        default=3,
    )
    parser.add_argument(
        "--num_distractor",
        help="Number of distractor paths",
        required=False,
        type=int,
        default=2,
    )
    parser.add_argument(
        "--distractor_length",
        help="Length of distractor paths",
        required=False,
        type=int,
        default=2,
    )
    parser.add_argument(
        "--export_path", help="Export path for file", required=True, type=str
    )
    args = parser.parse_args()

    manager = Manager()
    data = manager.dict()
    with Pool(32) as p:
        p.map(
            create_map,
            [
                (
                    data,
                    args.map_size,
                    args.goal_length,
                    args.num_distractor,
                    args.distractor_length,
                    i,
                )
                for i in range(args.num_train + args.num_test)
            ],
        )

    # Parse and save to file
    if not os.path.exists(args.export_path):
        os.makedirs(args.export_path)

    export_train_file = os.path.join(args.export_path, "train.txt")
    export_test_file = os.path.join(args.export_path, "test.txt")

    map_idx = 0
    with open(export_train_file, "w") as file:
        for _ in range(args.num_train):
            file.write(data[map_idx])
            map_idx += 1
            file.write("\n")

    with open(export_test_file, "w") as file:
        for _ in range(args.num_test):
            file.write(data[map_idx])
            map_idx += 1
            file.write("\n")


if __name__ == "__main__":
    main()
