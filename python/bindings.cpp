#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// 包含我们自己的Qt类型转换器
#include "pybind11_qt_casters.h"

#include "../src/core/GameEngine.h"
#include "../src/core/data/BaseBallData.h"

namespace py = pybind11;

PYBIND11_MODULE(gobigger_env, m) {
    m.doc() = "AI Devour Evolve - GoBigger Environment for Reinforcement Learning";
    
    // Action 结构体
    py::class_<Action>(m, "Action")
        .def(py::init<>())
        .def(py::init<float, float, int>())
        .def_readwrite("direction_x", &Action::direction_x)
        .def_readwrite("direction_y", &Action::direction_y)
        .def_readwrite("action_type", &Action::action_type);
    
    // GlobalState 结构体
    py::class_<GlobalState>(m, "GlobalState")
        .def(py::init<>())
        .def_readwrite("border", &GlobalState::border)
        .def_readwrite("total_frame", &GlobalState::total_frame)
        .def_readwrite("last_frame_count", &GlobalState::last_frame_count)
        .def_readwrite("leaderboard", &GlobalState::leaderboard);
    
    // PlayerState 结构体
    py::class_<PlayerState>(m, "PlayerState")
        .def(py::init<>())
        .def_readwrite("rectangle", &PlayerState::rectangle)
        .def_readwrite("food", &PlayerState::food)
        .def_readwrite("thorns", &PlayerState::thorns)
        .def_readwrite("spore", &PlayerState::spore)
        .def_readwrite("clone", &PlayerState::clone)
        .def_readwrite("score", &PlayerState::score)
        .def_readwrite("can_eject", &PlayerState::can_eject)
        .def_readwrite("can_split", &PlayerState::can_split);
    
    // Observation 结构体
    py::class_<Observation>(m, "Observation")
        .def(py::init<>())
        .def_readwrite("global_state", &Observation::global_state)
        .def_readwrite("player_states", &Observation::player_states);
    
    // GameEngine 类
    py::class_<GameEngine>(m, "GameEngine")
        .def(py::init<>())
        .def("reset", &GameEngine::reset, "Reset the game environment")
        .def("step", &GameEngine::step, "Execute one step with given action", py::arg("action"))
        .def("is_done", &GameEngine::isDone, "Check if the game is done")
        .def("get_observation", &GameEngine::getObservation, "Get current observation")
        .def("start_game", &GameEngine::startGame, "Start the game")
        .def("pause_game", &GameEngine::pauseGame, "Pause the game")
        .def("reset_game", &GameEngine::resetGame, "Reset the game")
        .def("is_game_running", &GameEngine::isGameRunning, "Check if game is running")
        .def("get_total_frames", &GameEngine::getTotalFrames, "Get total frame count")
        .def("get_total_player_score", &GameEngine::getTotalPlayerScore, 
             "Get total score for a player", py::arg("team_id"), py::arg("player_id"));
    
    // GameEngine::Config 结构体
    py::class_<GameEngine::Config>(m, "GameConfig")
        .def(py::init<>())
        .def_readwrite("initFoodCount", &GameEngine::Config::initFoodCount)
        .def_readwrite("maxFoodCount", &GameEngine::Config::maxFoodCount)
        .def_readwrite("initThornsCount", &GameEngine::Config::initThornsCount)
        .def_readwrite("maxThornsCount", &GameEngine::Config::maxThornsCount)
        .def_readwrite("foodRefreshFrames", &GameEngine::Config::foodRefreshFrames)
        .def_readwrite("thornsRefreshFrames", &GameEngine::Config::thornsRefreshFrames)
        .def_readwrite("foodRefreshPercent", &GameEngine::Config::foodRefreshPercent)
        .def_readwrite("thornsRefreshPercent", &GameEngine::Config::thornsRefreshPercent)
        .def_readwrite("thornsScoreMin", &GameEngine::Config::thornsScoreMin)
        .def_readwrite("thornsScoreMax", &GameEngine::Config::thornsScoreMax);
    
    // 添加一些实用函数
    m.def("create_action", [](float dx, float dy, int type) {
        return Action(dx, dy, type);
    }, "Create an action", py::arg("direction_x"), py::arg("direction_y"), py::arg("action_type"));
    
    // 版本信息
    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "AI Devour Evolve Team";
}
