#include "Player.h"
#include "Tile.h"

Player::Player()
{
	// isInControl = true;
	setupAnimations();
}

Player::Player(float x, float y)
	: Mob(x, y)
{
	isInControl = true;
	setupAnimations();
}

Player::Player(float x, float y, Level *level)
	: Mob(x, y, level)
{
	isInControl = true;
	setupAnimations();
}

Player::~Player()
{
}

void Player::setupAnimations()
{
	Log::info("Setting up Animations for player");
	m_NorthAnimation = std::make_unique<Render::AnimatedSprite>(2);
	m_NorthAnimation->addSprite(PLAYER_NORTH_1);
	m_NorthAnimation->addSprite(PLAYER_NORTH_2);

	m_SouthAnimation = std::make_unique<Render::AnimatedSprite>(2);
	m_SouthAnimation->addSprite(PLAYER_SOUTH_1);
	m_SouthAnimation->addSprite(PLAYER_SOUTH_2);

	m_EastAnimation = std::make_unique<Render::AnimatedSprite>(2);
	m_EastAnimation->addSprite(PLAYER_EAST_1);
	m_EastAnimation->addSprite(PLAYER_EAST_2);

	m_WestAnimation = std::make_unique<Render::AnimatedSprite>(2);
	m_WestAnimation->addSprite(PLAYER_WEST_1);
	m_WestAnimation->addSprite(PLAYER_WEST_2);
}

void Player::update()
{
	if(isInControl)
	{
		Vec2f ratio = {0, 0};
		if(Application::isKeyPressed(GLFW_KEY_W) || Application::isKeyPressed(GLFW_KEY_UP))
		{
			ratio.y += 1.0f;
		}
		if(Application::isKeyPressed(GLFW_KEY_S) || Application::isKeyPressed(GLFW_KEY_DOWN))
		{
			ratio.y -= 1.0f;
		}
		if(Application::isKeyPressed(GLFW_KEY_A) || Application::isKeyPressed(GLFW_KEY_LEFT))
		{
			ratio.x -= 1.0f;
		}
		if(Application::isKeyPressed(GLFW_KEY_D) || Application::isKeyPressed(GLFW_KEY_RIGHT))
		{
			ratio.x += 1.0f;
		}

		if(ratio.x != 0 || ratio.y != 0)
		{
			move(ratio);
		}
		else
			isMoving = false;

		if(isMoving)
		{
			switch(m_Dir)
			{
			case Direction::NORTH:
				m_NorthAnimation->update();
				break;
			case Direction::SOUTH:
				m_SouthAnimation->update();
				break;
			case Direction::EAST:
				m_EastAnimation->update();
				break;
			default:
				m_WestAnimation->update();
				break;
			}
		}
	}
}

void Player::render()
{
#define RENDER_ARGUMENTS x, y, 0.0f, Tile::TILE_SIZE * 1.25f
	if(isMoving)
	{
		switch(m_Dir)
		{
		case Direction::NORTH:
			m_NorthAnimation->render(RENDER_ARGUMENTS);
			break;
		case Direction::SOUTH:
			m_SouthAnimation->render(RENDER_ARGUMENTS);
			break;
		case Direction::EAST:
			m_EastAnimation->render(RENDER_ARGUMENTS);
			break;
		default:
			m_WestAnimation->render(RENDER_ARGUMENTS);
			break;
		}
	}
	else
	{
		switch(m_Dir)
		{
		case Direction::NORTH:
			Render::Sprite::getSprite(PLAYER_NORTH)->render(RENDER_ARGUMENTS);
			break;
		case Direction::SOUTH:
			Render::Sprite::getSprite(PLAYER_SOUTH)->render(RENDER_ARGUMENTS);
			break;
		case Direction::EAST:
			Render::Sprite::getSprite(PLAYER_EAST)->render(RENDER_ARGUMENTS);
			break;
		default:
			Render::Sprite::getSprite(PLAYER_WEST)->render(RENDER_ARGUMENTS);
			break;
		}
	}
}

bool Player::eventCallback(const Application::Event &e)
{
	return false;
}

#ifdef DEBUG
void Player::imGuiRender()
{
	ImGui::Checkbox("Ghost mode", &isGhost);
	ImGui::SliderFloat("Player Speed", &m_Speed, 0.0f, 100.0f);
}
#endif