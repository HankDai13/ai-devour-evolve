
from gobigger.server import Server
from gobigger.render import EnvRender

def create_gobigger_env():
    """
    Create a GoBigger environment with parameters matched to the C++ project.
    """
    server_cfg = dict(
        # --- Game Settings ---
        gametime=300,  # Corresponds to GAME_TIME_S
        map_width=500, # Corresponds to MAP_WIDTH
        map_height=500, # Corresponds to MAP_HEIGHT
        # --- Team and Player Settings ---
        team_num=4, # Corresponds to TEAM_NUM
        player_num_per_team=3, # Corresponds to PLAYER_NUM_PER_TEAM
        # --- Ball and Cell Settings ---
        player_init_score=100, # Corresponds to PLAYER_INIT_SCORE
        player_max_part_num=16, # Corresponds to PLAYER_MAX_PART_NUM
        player_max_score=30000, # Corresponds to PLAYER_MAX_SCORE
        player_merge_time=30, # Corresponds to PLAYER_MERGE_TIME
        # --- Food Settings ---
        food_num=2000, # Corresponds to FOOD_NUM
        food_refresh_num=200, # Corresponds to FOOD_REFRESH_NUM
        food_refresh_time=2, # Corresponds to FOOD_REFRESH_TIME
        # --- Thorn Settings ---
        thorn_num=15, # Corresponds to THORN_NUM
        thorn_eat_player_part_num=10, # Corresponds to THORN_EAT_PLAYER_PART_NUM
        # --- Action Settings ---
        eject_score_lose=20, # Corresponds to EJECT_SCORE_LOSE
        split_score_lose=18, # Corresponds to SPLIT_SCORE_LOSE
        # --- Observation Settings (as per AI_Interface_Spec.md) ---
        # You can choose different observation types. 'dict_spatial' is flexible.
        # It provides spatial features which are good for CNN-based models.
        obs_settings=dict(
            with_spatial=True,
            with_speed=True,
            with_all_vision=False,
        ),
    )
    server = Server(server_cfg)
    # To visualize the game, you can wrap it with EnvRender
    # render = EnvRender(server)
    # return render
    return server

if __name__ == '__main__':
    # Example of how to create and use the environment
    env = create_gobigger_env()
    print("GoBigger environment created successfully with custom config.")
    print("Action Space:", env.action_space_sample)
    print("Observation Space Shape:", env.observation_space_sample.shape)
    
    obs = env.reset()
    done = False
    while not done:
        # Replace this with your RL agent's action
        actions = {player.name: env.action_space_sample for player in env.players}
        obs, reward, done, info = env.step(actions=actions)
    print("Environment episode finished.")

