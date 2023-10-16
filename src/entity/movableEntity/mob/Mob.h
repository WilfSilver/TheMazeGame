#pragma once

#include "MovableEntity.h"

#include <vector>

#include "AnimatedSprite.h"
#include "Item.h"
#include "StatsMob.h"
#include "Utils.h"

class Weapon;

class Mob : public MovableEntity, public StatsMob
{
  protected:
	std::vector<Item *>   inventory;
	std::vector<Weapon *> m_Weapons;
	int                   m_CurrentWeapon;
	float                 m_Size;   // TODO: This should probably go further up in the chain

	std::unique_ptr<AnimatedSprite> m_NorthAnimation;
	std::unique_ptr<AnimatedSprite> m_SouthAnimation;
	std::unique_ptr<AnimatedSprite> m_EastAnimation;
	std::unique_ptr<AnimatedSprite> m_WestAnimation;

	bool isInControl = false;

  public:
	Mob();
	Mob(float x, float y);
	Mob(float x, float y, Level *level);
	Mob(float x, float y, Level *level, uint16_t spriteID);
	Mob(float x, float y, float speed, Level *level, uint16_t spriteID);
	virtual ~Mob();

	void pickUp(Item *item);
	void setupAnimations();

	virtual void render();
	virtual void update();
	virtual bool eventCallback(const Event::Event &e);

	virtual bool deleteMe() { return isDead(); }

	const std::vector<Weapon *> &getWeapons() { return m_Weapons; }
	int *                        getCurrentWeaponIndex() { return &m_CurrentWeapon; }

	void setIsInControl(bool i_isInControl) { isInControl = i_isInControl; }
#ifdef DEBUG
	virtual void imGuiRender() = 0;
#endif
};