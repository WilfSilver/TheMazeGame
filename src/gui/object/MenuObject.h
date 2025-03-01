#pragma once

#include <functional>

#include "event/Event.h"
#include "layer/Layer.h"

class MenuObject
{
  protected:
	// Stores position and size of the object
	float x, y;
	float width, height;

	std::function<void(float *, float *, float *, float *)> positionFunc;

	Layer *m_Layer;

  public:
	MenuObject(float x, float y, float width, float height, Layer *layer);
	MenuObject(std::function<void(float *, float *, float *, float *)> posFunc, Layer *layer);
	virtual ~MenuObject();

	virtual void render() = 0;
	virtual void update() = 0;
	virtual bool eventCallback(const Event::Event &e);

	float  getX() { return x; }
	float  getY() { return y; }
	Layer *getLayer() { return m_Layer; }
};