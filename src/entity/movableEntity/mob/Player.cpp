#include "Player.h"

#include "Projectile.h"
#include "Tile.h"

#include "Boomerang.h"
#include "Bow.h"
#include "CrossBow.h"
#include "DarkStaff.h"
#include "FireStaff.h"
#include "FrostStaff.h"
#include "GoldStaff.h"
#include "GoldStaff.h"
#include "EarthStaff.h"
#include "Sling.h"

Player::Player()
{
}

Player::Player(float x, float y)
	: Mob(x, y)
{
}

Player::Player(float x, float y, Level *level)
	: Mob(x, y, level)
{
	// changeMaxHealth(1000);
	// changeHealth(1000);
	m_Weapons.push_back(new FireStaff());
	m_CurrentWeapon = 0;

	m_Inventory.push_back(new FireStaff());
	m_Inventory.push_back(new FrostStaff());
	m_Inventory.push_back(new DarkStaff());
	m_Inventory.push_back(new GoldStaff());
	m_Inventory.push_back(new EarthStaff());
	m_Inventory.push_back(new Sling());
	m_Inventory.push_back(new Bow());
	m_Inventory.push_back(new Crossbow());
	m_Inventory.push_back(new Boomerang());
}

Player::~Player()
{
}

void Player::update()
{
	if(isInControl)
	{
		Vec2f ratio = {0, 0};
		if(Event::isKeyPressed(GLFW_KEY_W) || Event::isKeyPressed(GLFW_KEY_UP))
			ratio.y += 1.0f;
		if(Event::isKeyPressed(GLFW_KEY_S) || Event::isKeyPressed(GLFW_KEY_DOWN))
			ratio.y -= 1.0f;
		if(Event::isKeyPressed(GLFW_KEY_A) || Event::isKeyPressed(GLFW_KEY_LEFT))
			ratio.x -= 1.0f;
		if(Event::isKeyPressed(GLFW_KEY_D) || Event::isKeyPressed(GLFW_KEY_RIGHT))
			ratio.x += 1.0f;

		if(ratio.x != 0 || ratio.y != 0)
			move(ratio);
		else
			isMoving = false;

		Mob::update();
	}
}

void Player::render()
{
	Mob::render();
}

bool Player::eventCallback(const Event::Event &e)
{
	if(e.getType() == Event::EventType::keyInput)
	{
		const Event::KeyboardEvent &ne = static_cast<const Event::KeyboardEvent &>(e);
		if(ne.key == GLFW_KEY_SPACE && (ne.action == GLFW_PRESS || ne.action == GLFW_REPEAT) && m_CurrentWeapon != -1)
		{
			useCurrentWeapon(ne.action == GLFW_REPEAT);
			return true;
		}
	}
	return Mob::eventCallback(e);
}

void Player::setFollowing(Mob *following)
{
	Log::warning("Trying to set player follower!");
}

void Player::setEnemy(Mob *enemy)
{
	Log::warning("Trying to set player enemy!");
}

#ifdef DEBUG
void Player::imGuiRender()
{
	ImGui::Checkbox("Ghost mode", &isGhost);
	ImGui::SliderFloat("Player Speed", &m_Speed, 0.0f, 100.0f);
}
#endif