#pragma once

#include "MenuObject.h"

#include "Sprite.h"

class Image : public MenuObject
{
  private:
	Sprite::ID m_SpriteID;

  public:
	Image(float x, float y, float width, float height, Sprite::ID spriteID, Layer *layer);
	Image(std::function<void(float *, float *, float *, float *)> posFunc, Sprite::ID spriteID, Layer *layer);

	virtual void render() override;
	virtual void update() override;
};