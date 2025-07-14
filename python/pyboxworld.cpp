// pyboxworld.cpp
// Python bindings

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "boxworld/boxworld.h"

namespace py = pybind11;

PYBIND11_MODULE(pyboxworld, m) {
    m.doc() = "Boxworld environment module docs.";
    namespace bw = ::boxworld;
    using T = bw::BoxWorldGameState;

    py::enum_<bw::Element>(m, "BoxWorldElement")
        .value("kColour0", bw::Element::kColour0)
        .value("kColour1", bw::Element::kColour1)
        .value("kColour2", bw::Element::kColour2)
        .value("kColour3", bw::Element::kColour3)
        .value("kColour4", bw::Element::kColour4)
        .value("kColour5", bw::Element::kColour5)
        .value("kColour6", bw::Element::kColour6)
        .value("kColour7", bw::Element::kColour7)
        .value("kColour8", bw::Element::kColour8)
        .value("kColour9", bw::Element::kColour9)
        .value("kColour10", bw::Element::kColour10)
        .value("kColour11", bw::Element::kColour11)
        .value("kColourGoal", bw::Element::kColourGoal)
        .value("kAgent", bw::Element::kAgent)
        .value("kEmpty", bw::Element::kEmpty)
        .value("kWall", bw::Element::kWall);

    py::class_<T>(m, "BoxWorldGameState")
        .def(py::init<const std::string &>())
        .def_readonly_static("name", &T::name)
        .def_readonly_static("num_actions", &bw::kNumActions)
        .def(py::self == py::self)    // NOLINT (misc-redundant-expression)
        .def(py::self != py::self)    // NOLINT (misc-redundant-expression)
        .def("__hash__", [](const T &self) { return self.get_hash(); })
        .def("__copy__", [](const T &self) { return T(self); })
        .def("__deepcopy__", [](const T &self, py::dict) { return T(self); })
        .def("__repr__",
             [](const T &self) {
                 std::stringstream stream;
                 stream << self;
                 return stream.str();
             })
        .def(py::pickle(
            [](const T &self) {    // __getstate__
                auto s = self.pack();
                return py::make_tuple(s.rows, s.cols, s.agent_idx, s.zorb_hash, s.reward_signal, s.inventory, s.board,
                                      s.key_indices, s.lock_indices);
            },
            [](py::tuple t) -> T {    // __setstate__
                if (t.size() != 9) {
                    throw std::runtime_error("Invalid state");
                }
                T::InternalState s;
                s.rows = t[0].cast<int>();                                // NOLINT(*-magic-numbers)
                s.cols = t[1].cast<int>();                                // NOLINT(*-magic-numbers)
                s.agent_idx = t[2].cast<int>();                           // NOLINT(*-magic-numbers)
                s.zorb_hash = t[3].cast<uint64_t>();                      // NOLINT(*-magic-numbers)
                s.reward_signal = t[4].cast<uint64_t>();                  // NOLINT(*-magic-numbers)
                s.inventory = t[5].cast<int>();                           // NOLINT(*-magic-numbers)
                s.board = t[6].cast<std::vector<int>>();                  // NOLINT(*-magic-numbers)
                s.key_indices = t[7].cast<std::unordered_set<int>>();     // NOLINT(*-magic-numbers)
                s.lock_indices = t[8].cast<std::unordered_set<int>>();    // NOLINT(*-magic-numbers)
                return {std::move(s)};
            }))
        .def("apply_action",
             [](T &self, int action) {
                 if (action < 0 || action >= T::action_space_size()) {
                     throw std::invalid_argument("Invalid action.");
                 }
                 self.apply_action(static_cast<bw::Action>(action));
             })
        .def("is_solution", &T::is_solution)
        .def("is_terminal", &T::is_solution)
        .def("observation_shape", &T::observation_shape)
        .def("get_observation",
             [](const T &self) {
                 py::array_t<float> out = py::cast(self.get_observation());
                 return out.reshape(self.observation_shape());
             })
        .def("image_shape", &T::image_shape)
        .def("to_image",
             [](T &self) {
                 py::array_t<uint8_t> out = py::cast(self.to_image());
                 const auto obs_shape = self.observation_shape();
                 return out.reshape({static_cast<py::ssize_t>(obs_shape[1] * bw::SPRITE_HEIGHT),
                                     static_cast<py::ssize_t>(obs_shape[2] * bw::SPRITE_WIDTH),
                                     static_cast<py::ssize_t>(bw::SPRITE_CHANNELS)});
             })
        .def("get_reward_signal", &T::get_reward_signal)
        .def("get_agent_index", &T::get_agent_index)
        .def("has_key", &T::has_key)
        .def("set_key", &T::set_key);
}
