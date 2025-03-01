#pragma once

#include "Layer.h"

#include <memory>

#include "gui/object/MenuObject.h"
#include "gui/object/TransferObject.h"
#include "layer/level/Level.h"

class GUILayer : public Layer
{
  public:
	// Stores the different types of layers that can be created
	enum class Type
	{
		MainMenu,
		GameOverlay,
		PlayerInventory,
		ChestInventory,
		NPCInventory,
		NPCInteraction,
		ExitMenu,
		PlayerDeath,
		PlayerWin,
	};

  private:
	Level *                   m_ConnectedLevel;
	std::vector<MenuObject *> m_Objects;

  public:
	GUILayer();
	GUILayer(Level *connectedLevel);
	GUILayer(Type genType, Level *connectedLevel);
	virtual ~GUILayer() override;

	virtual void render() override;
	virtual void update() override;
	virtual bool eventCallback(const Event::Event &e) override;

	void addMenuObject(MenuObject *object);

	void setConnectedLevel(Level *level);

	void transferObject(TransferObject *obj);

	std::vector<MenuObject *> &getObjects() { return m_Objects; }
	Level *                    getConnectedLevel() { return m_ConnectedLevel; }

#ifdef DEBUG
	virtual void imGuiRender() override
	{
	}
#endif
};