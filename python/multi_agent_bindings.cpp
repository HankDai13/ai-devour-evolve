#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// 包含我们自己的Qt类型转换器
#include "pybind11_qt_casters.h"
#include "multi_agent_game_engine.h"

namespace py = pybind11;

PYBIND11_MODULE(gobigger_multi_env, m) {
    m.doc() = "AI Devour Evolve - Multi-Agent GoBigger Environment for Reinforcement Learning";
    
    // ============ 多智能体引擎配置 ============
    py::class_<MultiAgentGameEngine::Config>(m, "MultiAgentConfig")
        .def(py::init<>())
        .def_readwrite("maxFoodCount", &MultiAgentGameEngine::Config::maxFoodCount)
        .def_readwrite("initFoodCount", &MultiAgentGameEngine::Config::initFoodCount)
        .def_readwrite("maxThornsCount", &MultiAgentGameEngine::Config::maxThornsCount)
        .def_readwrite("initThornsCount", &MultiAgentGameEngine::Config::initThornsCount)
        .def_readwrite("gameUpdateInterval", &MultiAgentGameEngine::Config::gameUpdateInterval)
        .def_readwrite("maxFrames", &MultiAgentGameEngine::Config::maxFrames)
        .def_readwrite("aiOpponentCount", &MultiAgentGameEngine::Config::aiOpponentCount);
    
    // ============ 多智能体游戏引擎 ============
    py::class_<MultiAgentGameEngine>(m, "MultiAgentGameEngine")
        .def(py::init<const MultiAgentGameEngine::Config&>(), 
             "创建多智能体游戏引擎")
        .def("reset", &MultiAgentGameEngine::reset, 
             "重置游戏环境，返回初始观察")
        .def("step", &MultiAgentGameEngine::step, 
             "执行一步动作，返回新的观察", py::arg("actions"))
        .def("isDone", &MultiAgentGameEngine::isDone, 
             "检查游戏是否结束")
        .def("is_done", &MultiAgentGameEngine::isDone, 
             "检查游戏是否结束 (Python别名)")
        .def("getObservation", &MultiAgentGameEngine::getObservation,
             "获取当前观察")
        .def("getRewardInfo", &MultiAgentGameEngine::getRewardInfo,
             "获取奖励信息")
        .def("get_reward_info", &MultiAgentGameEngine::getRewardInfo,
             "获取奖励信息 (Python别名)");
}
