# stub file for editor to resolve gobigger_env extension module
# The actual implementation is in the compiled extension (gobigger_env.pyd under build/Release)

__version__: str
__author__: str

from typing import Any, Tuple, Dict

class Action:
    direction_x: float
    direction_y: float
    action_type: int
    def __init__(self, direction_x: float, direction_y: float, action_type: int) -> None: ...

class GlobalState:
    border: Any
    total_frame: int
    last_frame_count: int
    leaderboard: Any

class PlayerState:
    rectangle: Tuple[float, ...]
    food: Tuple[Tuple[float, ...], ...]
    thorns: Tuple[Tuple[float, ...], ...]
    spore: Tuple[Tuple[float, ...], ...]
    clone: Tuple[Tuple[float, ...], ...]
    score: float
    can_eject: bool
    can_split: bool

class Observation:
    global_state: GlobalState
    player_states: Dict[Any, PlayerState]

class GameEngine:
    def __init__(self) -> None: ...
    def reset(self) -> Observation: ...
    def step(self, action: Action) -> Observation: ...
    def is_done(self) -> bool: ...
    def is_game_running(self) -> bool: ...
    def get_total_frames(self) -> int: ...


def clamp_action(action: Action) -> Action: ...

def create_action(dx: float, dy: float, action_type: int) -> Action: ...
