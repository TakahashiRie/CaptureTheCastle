//
// Created by Owner on 2019-10-12.
//


#include "player_input_system.hpp"

extern ECSManager ecsManager;

void PlayerInputSystem::init(std::shared_ptr<Tilemap> tilemap)
{
	ecsManager.subscribe(this, &PlayerInputSystem::onKeyListener);
	ecsManager.subscribe(this, &PlayerInputSystem::onReleaseListener);
	ecsManager.subscribe(this, &PlayerInputSystem::onTimeoutListener);
	bomb_set_sound = Mix_LoadWAV(audio_path("capturethecastle_set_bomb.wav"));
	soldier_set_sound = Mix_LoadWAV(audio_path("capturethecastle_set_soldier.wav"));
	m_tilemap = tilemap;
	flag = false;
}

void PlayerInputSystem::update()
{
	for (auto& e : entities)
    {
		auto& motion = ecsManager.getComponent<Motion>(e);
		auto& transform = ecsManager.getComponent<Transform>(e);
		auto& team = ecsManager.getComponent<Team>(e);
		auto& item = ecsManager.getComponent<ItemComponent>(e);
		vec2 next_dir = { 0, 0 };
		if (team.assigned == TeamType::PLAYER1)
		{
			for (auto key : PLAYER1KEYS)
			{
				if (keysPressed[key])
				{
					Tile tile = m_tilemap->get_tile(transform.position.x, transform.position.y);
					switch (key)
					{
					case InputKeys::W:
						next_dir.y -= 1;
						break;
					case InputKeys::S:
						next_dir.y += 1;
						break;
					case InputKeys::D:
						next_dir.x += 1;
						break;
					case InputKeys::A:
						next_dir.x -= 1;
						break;
					case InputKeys ::LEFT_SHIFT:
                        if (item.itemType == ItemType::BOMB){
                            place_bomb(tile, TeamType::PLAYER1);
                            ecsManager.publish(new ItemEvent(e, ItemType::BOMB, false));
                            item.itemType = ItemType::None;
                            Mix_PlayChannel(-1, bomb_set_sound, 0);
                        }
					    break;
					case InputKeys::Q:
						handle_soldier_spawn(
							soldier_count_1, wait_1, transform, tile,
							MazeRegion::PLAYER1, TeamType::PLAYER1,
							textures_path("red_soldier_sprite_sheet-01.png")
						);
						break;
					default:
						break;
					}
				}
			}
			++wait_1;
		}
		else if (team.assigned == TeamType::PLAYER2)
		{
			for (auto key : PLAYER2KEYS)
			{
				if (keysPressed[key])
				{
					Tile tile = m_tilemap->get_tile(transform.position.x, transform.position.y);
					switch (key)
					{
					case InputKeys::UP:
						next_dir.y -= 1;
						break;
					case InputKeys::DOWN:
						next_dir.y += 1;
						break;
					case InputKeys::RIGHT:
						next_dir.x += 1;
						break;
					case InputKeys::LEFT:
						next_dir.x -= 1;
						break;
					case InputKeys::SLASH:
						handle_soldier_spawn(
							soldier_count_2, wait_2, transform, tile,
							MazeRegion::PLAYER2, TeamType::PLAYER2,
							textures_path("blue_soldier_sprite_sheet-01.png")
						);
						break;
                    case InputKeys ::RIGHT_SHIFT:
                        if (item.itemType == ItemType::BOMB){
                            place_bomb(tile, TeamType::PLAYER2);
                            ecsManager.publish(new ItemEvent(e, ItemType::BOMB, false));
                            item.itemType = ItemType::None;
                            Mix_PlayChannel(-1, bomb_set_sound, 0);
                        }
                        break;
					default:
						break;
					}
				}
			}
			++wait_2;
		}
		motion.direction = next_dir;
		if (e == playerWithFlag && flag)
		{
			auto& bubbMotion = ecsManager.getComponent<Motion>(bubble);
			bubbMotion.direction = next_dir;
		}
	}
}

void PlayerInputSystem::handle_soldier_spawn(
	size_t& soldier_count, size_t& wait_time, const Transform& transform, const Tile& tile,
	const MazeRegion& maze_region, const TeamType& team_type, const char* texture_path
)
{
	if (can_spawn(soldier_count, wait_time, transform, tile, maze_region))
	{
		vec2 position = tile.get_position();
		Transform transform_soldier = Transform{ position, position,{ 0.08f * 5 / 7, 0.08f }, position };
		Motion motion_soldier = Motion{ { 0, 0 }, 100.f };
		spawn_soldier(
			transform_soldier, motion_soldier,
			team_type, texture_path
		);
        Mix_PlayChannel(-1, soldier_set_sound, 0);
		++soldier_count;
		wait_time = 0;
	}
}

bool PlayerInputSystem::can_spawn(
	const size_t& soldier_count, const size_t& wait_time, const Transform& transform,
	const Tile& tile, const MazeRegion& maze_region
)
{
	const std::pair<int, int> tile_idx = tile.get_idx();
	return (
		(!tile.is_wall()) &&
		(soldier_count < MAX_SOLDIERS) &&
		(wait_time > RESPAWN_IDLE) &&
		(tile_idx.first > 3 && tile_idx.first < 15) &&
		(tile_idx.second > 4 && tile_idx.second < 23) && // Right at the opening edge
		(m_tilemap->get_region(transform.position.x, transform.position.y) == maze_region)
	);
}

void PlayerInputSystem::onKeyListener(InputKeyEvent* input)
{
	keysPressed[input->key] = true;
}

void PlayerInputSystem::onReleaseListener(KeyReleaseEvent* input)
{
	keysPressed[input->keyReleased] = false;
}

void PlayerInputSystem::reset() {
    keysPressed.clear();
	this->entities.clear();
    if (bomb_set_sound != nullptr) {
        Mix_FreeChunk(bomb_set_sound);
    }
    if (soldier_set_sound != nullptr) {
        Mix_FreeChunk(soldier_set_sound);
    }
}

void PlayerInputSystem::place_bomb(const Tile& tile, const TeamType& team_type) {
    Entity bomb = ecsManager.createEntity();
    ecsManager.addComponent<Team>(bomb, Team{ team_type });
    ecsManager.addComponent<PlaceableComponent>(bomb, PlaceableComponent{});
    vec2 position = tile.get_position();
    Transform transform = Transform{ position, position,{ 0.8f, 0.8f }, position };
    ecsManager.addComponent<Transform>(bomb, transform);
    ecsManager.addComponent<ItemComponent>(bomb, ItemComponent{
            true,
            ItemType::BOMB});
    ecsManager.addComponent<CurveMotionComponent>(bomb, CurveMotionComponent{
        vec2{0.f, 0.f}, vec2{6.f, 20.f}, vec2{25.f, 20.f}, vec2{30.f, 0.f}
    });
    EffectComponent itemEffect{
        shader_path("textured.vs.glsl"), shader_path("textured.fs.glsl")
    };
    EffectManager::instance().load_from_file(itemEffect);
    ecsManager.addComponent<EffectComponent>(bomb, itemEffect);
    Sprite itemSprite;
    if (team_type == TeamType ::PLAYER1){
        itemSprite = {power_up_path("CaptureTheCastle_powerup_bomb_setRed.png")};
    } else if (team_type == TeamType::PLAYER2){
        itemSprite = {power_up_path("CaptureTheCastle_powerup_bomb_setBlue.png")};
    }
    TextureManager::instance().load_from_file(itemSprite);
    ecsManager.addComponent<Sprite>(bomb, itemSprite);
    MeshComponent itemMesh{MeshManager::instance().init_mesh(
            itemSprite.width, itemSprite.height)};
    ecsManager.addComponent<MeshComponent>(bomb, itemMesh);
    float radius = itemSprite.width/2*0.8f;
    float i_width = itemSprite.width*0.8f;
    float i_height = itemSprite.height*0.8f;
    ecsManager.addComponent<C_Collision>(bomb, C_Collision{
            CollisionLayer::Item,
            radius,
            {i_width, i_height}
    });
}

void PlayerInputSystem::spawn_soldier(
	const Transform& transform, const Motion& motion,
	const TeamType& team_type, const char* texture_path
)
{
	Entity soldier = ecsManager.createEntity();
	ecsManager.addComponent<Team>(soldier, Team{ team_type });
	ecsManager.addComponent<Transform>(soldier, transform);
	ecsManager.addComponent<Motion>(soldier, motion);

	ecsManager.addComponent<SoldierAiComponent>(
		soldier,
		SoldierAiComponent{
			SoldierState::IDLE,
			0,0,
			vec2{ 0.f,0.f }
		}
	);

	EffectComponent soldierEffect{
	    shader_path("textured.vs.glsl"), shader_path("textured.fs.glsl")
	};
	EffectManager::instance().load_from_file(soldierEffect);
	ecsManager.addComponent<EffectComponent>(soldier, soldierEffect);
	Sprite soldierSprite = { texture_path };
	TextureManager::instance().load_from_file(soldierSprite);
	soldierSprite.sprite_index = { 0 , 0 };
	soldierSprite.sprite_size = { soldierSprite.width / 7.0f , soldierSprite.height / 5.0f };
	ecsManager.addComponent<Sprite>(soldier, soldierSprite);
	MeshComponent soldierMesh{};
	soldierMesh.id = MeshManager::instance().init_mesh(
		soldierSprite.width, soldierSprite.height, soldierSprite.sprite_size.x, soldierSprite.sprite_size.y,
		soldierSprite.sprite_index.x, soldierSprite.sprite_index.y, 0
	);
	ecsManager.addComponent<MeshComponent>(soldier, soldierMesh);
	ecsManager.addComponent(
		soldier,
		C_Collision{
			CollisionLayer::Enemy,
			soldierSprite.width / 2 * 0.08f,
			{ soldierSprite.width * 0.08f * 0.8f, soldierSprite.height * 0.08f * 0.8f }
		}
	);
}

void PlayerInputSystem::onTimeoutListener(TimeoutEvent *input) {
	for (auto& e : entities)
	{
		auto& transform = ecsManager.getComponent<Transform>(e);
		transform.position = transform.init_position;
	}
}

void PlayerInputSystem::setFlagMode(bool mode, Entity flagPlayer, Entity bubb)
{
	flag = mode;
	playerWithFlag = flagPlayer;
	bubble = bubb;
}