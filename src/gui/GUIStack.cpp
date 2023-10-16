#include "GUIStack.h"

#include "Application.h"

GUIStack::GUIStack(Level *level)
	: m_ActiveLayer(0)
{
	m_Layers.push_back(new GUILayer(GUILayer::Type::GameOverlay, level));
	m_Layers.push_back(new GUILayer(GUILayer::Type::PlayerInventory, level));
	m_Layers.push_back(new GUILayer(GUILayer::Type::ChestInventory, level));
	m_Layers.push_back(new GUILayer(GUILayer::Type::NPCInventory, level));
	m_Layers.push_back(new GUILayer(GUILayer::Type::NPCInteraction, level));
	m_Layers.push_back(new GUILayer(GUILayer::Type::ExitMenu, level));
	m_Layers.push_back(new GUILayer(GUILayer::Type::PlayerDeath, level));
	m_Layers.push_back(new GUILayer(GUILayer::Type::PlayerWin, level));
}

GUIStack::~GUIStack()
{
	for(GUILayer *layer : m_Layers)
		delete layer;
}

void GUIStack::render()
{
	if(m_ActiveLayer != -1)
		m_Layers[m_ActiveLayer]->render();
}

void GUIStack::update()
{
	if(m_ActiveLayer != -1)
		m_Layers[m_ActiveLayer]->update();
}

bool GUIStack::eventCallback(const Event::Event &e)
{
	if(e.getType() == Event::EventType::changeGUILayer)
	{
		const Event::ChangeGUIActiveLayer &ne = static_cast<const Event::ChangeGUIActiveLayer &>(e);

		m_ActiveLayer = static_cast<int>(ne.layer);

		if(ne.layer == InGameGUILayer::overlay)
			Application::setIsPaused(false);
		else
			Application::setIsPaused(true);

		return true;
	}
	else if(e.getType() == Event::EventType::windowResize)
	{
		for(GUILayer *layer : m_Layers)
			layer->eventCallback(e);
	}
	else
		return m_Layers[m_ActiveLayer]->eventCallback(e);

	return false;
}
