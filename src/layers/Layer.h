#pragma once

#include "Event.h"
#include "RenderEffect.h"
#include "Renderer.h"

#include <memory>

class Layer
{
  protected:
	std::unique_ptr<Shader> m_Shader;
	std::unique_ptr<Render::SmartBuffer> m_Buffer;

  public:
	Layer() {}
	virtual ~Layer() {}

	virtual void render()                                   = 0;
	virtual void update()                                   = 0;
	virtual bool eventCallback(const Application::Event &e) = 0;

	bool setEffect(const Effect::RenderEffect &e)
	{
		e.setEffect(*m_Shader);
		return false;
	}

#ifdef DEBUG
	virtual void imGuiRender() = 0;
#endif
};