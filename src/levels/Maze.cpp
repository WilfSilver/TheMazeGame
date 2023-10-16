#include "Maze.h"

#include <ImGui.h>
#include <algorithm>
#include <string>

#include "AStarUtils.h"
#include "Application.h"
#include "Log.h"
#include "RandomGen.h"
#include "VertexBufferLayout.h"

#include "Button.h"
#include "GUILayer.h"
#include "MenuBackgroundObject.h"
#include "MenuItemHolderManager.h"
#include "StatBar.h"

#include "EmptyRoom.h"
#include "NPC.h"
#include "Tile.h"

#include "WorldItem.h"

#include "FireStaff.h"
#include "Potion.h"

#define LAYER_MAX_FOR_DIRECTIONS 4

Maze::Maze()
	: Level(BOARD_SIZE * ROOM_SIZE, BOARD_SIZE * ROOM_SIZE),
	  xoffset(0),
	  yoffset(0),
	  finishedGenerating(true),
	  m_Player(((float) BOARD_SIZE / 2.0f) * ROOM_SIZE * Tile::TILE_SIZE - Tile::TILE_SIZE / 2, ((float) BOARD_SIZE / 2.0f) * ROOM_SIZE * Tile::TILE_SIZE - Tile::TILE_SIZE / 2, this),
	  m_OverlayGUI(new GUILayer(this)),
	  m_InventoryGUI(new GUILayer(this))
{
	// NOTE: Because of how it is rendering the coords (0,0) on the board is the bottom left, not the top left!!

	board.resize(BOARD_SIZE * BOARD_SIZE, nullptr);   // It is resized, because all positions are used straight away and fills any data slots will nullptr
	currentPaths.reserve(2 * BOARD_SIZE);             // The data is reserved here because not all the data is needed, but it could be used and so for efficiency, it is reserved on init

	Application::getCamera()->setAnchor(&m_Player);

	NPC *enemy = new NPC(3100.0f, 3800.0f, this);
	enemy->setFollower(&m_Player);
	// follower->setFollower(&m_Player);
	m_Entities.push_back(enemy);

	Item *     item      = new FireStaff();
	WorldItem *worldItem = new WorldItem(3800.0f, 3800.0f, Tile::TILE_SIZE / 2, this, item);
	m_Entities.push_back(worldItem);
	Item *     item2      = new FireStaff();
	WorldItem *worldItem2 = new WorldItem(3900.0f, 3800.0f, Tile::TILE_SIZE / 2, this, item2);
	m_Entities.push_back(worldItem2);

	auto healthPotionFunc = [](Mob *mob) {
		mob->changeHealth(100);
	};

	Item *     potion     = new Potion("Large Health Potion", POTION_HEALTH, healthPotionFunc);
	WorldItem *worldItem3 = new WorldItem(3800.0f, 3900.0f, Tile::TILE_SIZE / 2, this, potion);
	m_Entities.push_back(worldItem3);

	// Overlay GUI set up
	{
		auto clickedFunc = [](int index, Level *level) {
			level->getPlayer()->setCurrentWeapon(index);
		};
		m_OverlayGUI->addMenuObject(new MIHManager(0, 0, 300, 100, 100, m_OverlayGUI, (std::vector<Item *> &) m_Player.getWeapons(), clickedFunc, m_Player.getCurrentWeaponPointer()));

		auto posFunc = [](float *x, float *y, float *width, float *height) {
			*x      = Application::getWidth() / 2;
			*y      = 20;
			*width  = Application::getWidth() / 3;
			*height = 10;
		};
		m_OverlayGUI->addMenuObject(new StatBar(posFunc, m_OverlayGUI, m_Player.getHealthPointer(), m_Player.getMaxHealthPointer()));
		Application::addOverlay(m_OverlayGUI);
		activeGUI = m_OverlayGUI;
	}

	// Inventory GUI set up
	{
		auto backgroundPosFunc = [](float *x, float *y, float *width, float *height) {
			*width  = 550;
			*height = 575;
			*x      = Application::getWidth() / 2;
			*y      = Application::getHeight() / 2 + 75.0f / 2.0f;
		};
		auto exitFunc = [this]() mutable {
			Application::removeLayer(activeGUI);
			Application::addOverlay(m_OverlayGUI);
			activeGUI = m_OverlayGUI;
			Application::setIsPaused(false);
		};
		m_InventoryGUI->addMenuObject(new MenuBackground(backgroundPosFunc, m_InventoryGUI, {0.3f, 0.3f, 0.3f, 0.9f}, exitFunc));

		auto clickedFunc = [this](int index, Level *level) {   // TODO: remove the level *
			this->m_Player.useItemInInventory(index);
		};

		auto posFunc = [](float *x, float *y, float *width, float *height) {
			*width  = 500;
			*height = 400;
			*x      = Application::getWidth() / 2 - *width / 2;
			*y      = Application::getHeight() / 2 + *height / 2 - 112.5f;
		};
		m_InventoryGUI->addMenuObject(new MIHManager(posFunc, 100, m_InventoryGUI, m_Player.getInventory(), clickedFunc));

		auto clickedWeaponFunc = [this](int index, Level *level) {
			this->m_Player.setCurrentWeapon(index);
		};
		auto posWeaponFunc = [](float *x, float *y, float *width, float *height) {
			*width  = 300;
			*height = 100;
			*x      = Application::getWidth() / 2 - *width / 2;
			*y      = Application::getHeight() / 2 + 200;
		};
		m_InventoryGUI->addMenuObject(new MIHManager(posWeaponFunc, 100, m_OverlayGUI, (std::vector<Item *> &) m_Player.getWeapons(), clickedWeaponFunc, m_Player.getCurrentWeaponPointer()));
	}

	Log::info("Maze initialised");
}

Maze::~Maze()
{
	// NOTE: This needs to be caused before the progam ends as it frees up the memory
	for(int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
	{
		if(board[i])
			delete board[i];
	}
	// for(Entity *entity : m_Entities)
	// delete entity;
	if(activeGUI != m_OverlayGUI)
		delete m_OverlayGUI;
	if(activeGUI != m_InventoryGUI)
		delete m_InventoryGUI;
	Log::info("Maze destroyed");
}

// Level overrides
void Maze::render()
{
	// m_Shader->bind();
	float multiple = ROOM_SIZE * Tile::TILE_SIZE;
	int   midpoint = BOARD_SIZE / 2;
#ifdef DEBUG
	if(renderAll)
	{   // This is to allow the option to render all - however only when debugging - because of the limit with vertices, they must be rendered in blocks
		int c = 0;
		for(int y = 0; y < BOARD_SIZE; y++)
		{
			for(int x = 0; x < BOARD_SIZE; x++)
			{
				Room *room = get(y, x);
				if(room)
				{
					room->render(x * multiple, y * multiple);
					c++;
				}
			}
		}
	}
	else
	{
		Room *mid = get(midpoint, midpoint);
		mid->render(midpoint * multiple, midpoint * multiple);
		if(mid->isOpen(0) && get(midpoint + 1, midpoint))
			get(midpoint + 1, midpoint)->render(midpoint * multiple, (midpoint + 1) * multiple);
		if(mid->isOpen(1) && get(midpoint - 1, midpoint))
			get(midpoint - 1, midpoint)->render(midpoint * multiple, (midpoint - 1) * multiple);
		if(mid->isOpen(2) && get(midpoint, midpoint + 1))
			get(midpoint, midpoint + 1)->render((midpoint + 1) * multiple, midpoint * multiple);
		if(mid->isOpen(3) && get(midpoint, midpoint - 1))
			get(midpoint, midpoint - 1)->render((midpoint - 1) * multiple, midpoint * multiple);
	}
#else
	Room *mid = get(midpoint, midpoint);
	mid->render(midpoint * multiple, midpoint * multiple);
	if(mid->isOpen(0))
		get(midpoint + 1, midpoint)->render(midpoint * multiple, (midpoint + 1) * multiple);
	if(mid->isOpen(1))
		get(midpoint - 1, midpoint)->render(midpoint * multiple, (midpoint - 1) * multiple);
	if(mid->isOpen(2))
		get(midpoint, midpoint + 1)->render((midpoint + 1) * multiple, midpoint * multiple);
	if(mid->isOpen(3))
		get(midpoint, midpoint - 1)->render((midpoint - 1) * multiple, midpoint * multiple);

#endif
	Render::render(m_ShaderEffectsIDs);

	Render::orderBuffersByYAxis();

	for(Entity *entity : m_Entities)
	{
		if(Application::isInFrame(entity->getX(), entity->getY(), entity->getCollisionBox()))
			entity->render();
	}

	for(Projectile *projectile : m_Projectiles)
	{
		if(Application::isInFrame(projectile->getX(), projectile->getY(), projectile->getCollisionBox()))
			projectile->render();
	}

	m_Player.render();
}

void Maze::update()
{
	if(!finishedGenerating)
		return;
	if(m_Player.getY() > ((BOARD_SIZE / 2) + 1) * Tile::TILE_SIZE * ROOM_SIZE)
	{
		Log::info("Player moved North");

		moveNorth();
		Event::MazeMovedEvent e(0.0f, (float) -Tile::TILE_SIZE * ROOM_SIZE);
		Application::callEvent(e, true);
	}
	else if(m_Player.getY() < ((BOARD_SIZE / 2)) * Tile::TILE_SIZE * ROOM_SIZE)
	{
		Log::info("Player moved South");

		moveSouth();
		Event::MazeMovedEvent e(0.0f, (float) Tile::TILE_SIZE * ROOM_SIZE);
		Application::callEvent(e, true);
	}
	if(m_Player.getX() > ((BOARD_SIZE / 2) + 1) * Tile::TILE_SIZE * ROOM_SIZE)
	{
		Log::info("Player moved East");

		moveEast();
		Event::MazeMovedEvent e((float) -Tile::TILE_SIZE * ROOM_SIZE, 0.0f);
		Application::callEvent(e, true);
	}
	else if(m_Player.getX() < (BOARD_SIZE / 2) * Tile::TILE_SIZE * ROOM_SIZE)
	{
		Log::info("Player moved West");

		moveWest();
		Event::MazeMovedEvent e((float) Tile::TILE_SIZE * ROOM_SIZE, 0.0f);
		Application::callEvent(e, true);
	}

	m_Player.update();

	for(auto it = m_Entities.begin(); it != m_Entities.end();)
	{
		(*it)->update();
		if((*it)->deleteMe() || (*it)->getX() < 0 || (*it)->getX() > width * Tile::TILE_SIZE || (*it)->getY() < 0 || (*it)->getY() > height * Tile::TILE_SIZE)
		{
			delete *it;
			it = m_Entities.erase(it);
		}
		else
			++it;
	}

	for(auto it = m_Projectiles.begin(); it != m_Projectiles.end();)
	{
		(*it)->update();
		if((*it)->deleteMe() || (*it)->getX() < 0 || (*it)->getX() > width * Tile::TILE_SIZE || (*it)->getY() < 0 || (*it)->getY() > height * Tile::TILE_SIZE)
		{
			delete *it;
			it = m_Projectiles.erase(it);
		}
		else
			++it;
	}

	if(finishedGenerating)
	{
		bool noEntrances = true;
		for(int i = 0; i < BOARD_SIZE; i++)
		{
			if(pathsNorth[i] || pathsSouth[i] || pathsEast[i] || pathsWest[i])
			{
				noEntrances = false;
				break;
			}
		}

		if(noEntrances)
			resetMaze();
	}

	int midpoint = BOARD_SIZE / 2 + 1;
	for(int x = midpoint - 1; x < midpoint + 2; x++)
	{
		for(int y = midpoint - 1; y < midpoint + 2; y++)
		{
			if(get(y, x))
				get(y, x)->update();
		}
	}

	if(m_Player.deleteMe())
		playerDeath();
}

#ifdef DEBUG
void Maze::imGuiRender()
{
	if(ImGui::Button("Reload Maze"))
	{
		generate();
	}
	ImGui::Checkbox("Render all", &renderAll);
	m_Player.imGuiRender();
}
#endif

bool Maze::eventCallback(const Event::Event &e)
{
	if(m_Player.eventCallback(e))
		return true;

	for(Projectile *p : m_Projectiles)
	{
		if(p->eventCallback(e))
			return true;
	}
	for(Entity *entity : m_Entities)
	{
		if(entity->eventCallback(e))
			return true;
	}

	if(e.getType() == Event::EventType::mazeMovedEvent)
	{
		int midpoint = BOARD_SIZE / 2 + 1;
		Room *activeRoom = get(midpoint, midpoint);
		if(!activeRoom)
			Log::error("The active room does not exist", LOGINFO);
		else
			activeRoom->activeRoom();
	}
	else if(e.getType() == Event::EventType::keyInput)
	{
		const Event::KeyboardEvent &ne = static_cast<const Event::KeyboardEvent &>(e);
		if(activeGUI != m_InventoryGUI && ne.key == GLFW_KEY_E && ne.action == GLFW_PRESS)
		{
			Application::removeLayer(activeGUI);
			Application::addOverlay(m_InventoryGUI);
			activeGUI = m_InventoryGUI;

			Application::setIsPaused(true);
		}
		else if(activeGUI != m_OverlayGUI && ne.key == GLFW_KEY_ESCAPE && ne.action == GLFW_PRESS)
		{
			Application::removeLayer(activeGUI);
			Application::addOverlay(m_OverlayGUI);
			activeGUI = m_OverlayGUI;
			Application::setIsPaused(false);
		}
	}
	else if(e.getType() == Event::EventType::windowResize)
	{
		if(activeGUI != m_OverlayGUI)
			m_OverlayGUI->eventCallback(e);
		if(activeGUI != m_InventoryGUI)
			m_InventoryGUI->eventCallback(e);
	}

	return false;
}

void Maze::playerDeath()
{
	Log::info("Player has died");
	m_Player.resetStats();
	resetMaze();
}

void Maze::resetMaze()
{
	Log::info("Resetting the maze!");
	for(Entity *e : m_Entities)
		delete e;
	m_Entities.clear();

	for(Projectile *p : m_Projectiles)
		delete p;
	m_Projectiles.clear();

	for(int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
	{
		if(board[i])
		{
			delete board[i];
			board[i] = nullptr;
		}
	}

	generate();
}

// SECTION: Rooms
int Maze::coordsToIndex(int x, int y)
{
	if(x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
	{
		Log::error("Tried to get index out of bounds", LOGINFO);
		return -1;
	}
	int xCoord = x + xoffset;
	int yCoord = y + yoffset;
	if(xCoord >= BOARD_SIZE)
		xCoord -= BOARD_SIZE;
	if(yCoord >= BOARD_SIZE)
		yCoord -= BOARD_SIZE;
	return yCoord * BOARD_SIZE + xCoord;
}

void Maze::addRoom(int x, int y, bool north, bool south, bool east, bool west)
{
	// TODO: Add more randomization for the different types of rooms as well as entities and objects
	bool entrances[4]          = {north, south, east, west};
	board[coordsToIndex(x, y)] = new EmptyRoom(entrances);
}

void Maze::removeRoom(int y, int x)
{
	// TODO: take any entities out of the room!
	if(x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
	{
		Log::error("Tried to set room out of bounds", LOGINFO);
		return;
	}
	delete board[coordsToIndex(x, y)];
	board[coordsToIndex(x, y)] = nullptr;
}
std::vector<Vec2f> *Maze::getPath(Vec2f startPos, Vec2f dest, CollisionBox box)
{

	Vec2i relativeStart = {(int) startPos.x / (ROOM_SIZE * Tile::TILE_SIZE), (int) startPos.y / (ROOM_SIZE * Tile::TILE_SIZE)};
	Vec2i relativeDest  = {(int) dest.x / (ROOM_SIZE * Tile::TILE_SIZE), (int) dest.y / (ROOM_SIZE * Tile::TILE_SIZE)};

	if(relativeStart != relativeDest)
	{
		if(!get(relativeStart.y, relativeStart.x) || !get(relativeDest.y, relativeDest.x))
		{
			Log::warning("Entity or destination in a room that does not exist!");
			std::vector<Vec2f> *path = new std::vector<Vec2f>();
			path->push_back(dest);
			return path;
		}

		std::array<Vec2i, 4> offsets;
		offsets[0] = {0, 1};
		offsets[1] = {0, -1};
		offsets[2] = {1, 0};
		offsets[3] = {-1, 0};

		std::function<bool(int, int, int, int, CollisionBox)> collisionDetection = [this](int x, int y, int xs, int ys, CollisionBox box) -> bool {
			if(!get(y + ys, x + xs) /* || !get(y, x)*/)
				return true;
			else if(ys == 1)
				return !get(y, x)->isOpen(Direction::north);
			else if(ys == -1)
				return !get(y, x)->isOpen(Direction::south);
			else if(xs == 1)
				return !get(y, x)->isOpen(Direction::east);
			else if(xs == -1)
				return !get(y, x)->isOpen(Direction::west);
			else
				return false;
		};
		std::function<Vec2f(Vec2i)> convert = [](Vec2i vec) -> Vec2f {
			return {(float) vec.x * ROOM_SIZE * Tile::TILE_SIZE, (float) vec.y * ROOM_SIZE * Tile::TILE_SIZE};
		};
		std::vector<Vec2f> *myPath = aStarAlgorithm<BOARD_SIZE, BOARD_SIZE, 4>(relativeStart, relativeDest, box, offsets, collisionDetection, convert, BOARD_SIZE * BOARD_SIZE);

		float mid = ((float) Tile::TILE_SIZE * ROOM_SIZE) / 2;
		dest      = {(float) (*myPath)[myPath->size() - 1].x + mid, (float) (*myPath)[myPath->size() - 1].y + mid};

		delete myPath;
	}

	std::vector<Vec2f> *path = Level::getPath(startPos, dest, box);
	return path;
}
// !SECTION

// SECTION: Generation
void Maze::generatePaths(int layerMax, int startMax)
{
	// Log::info("Generating paths");
	int layer = 0;

	// Resets the paths north
	for(int i = 0; i < BOARD_SIZE; i++)
	{
		pathsNorth[i] = false;
		pathsSouth[i] = false;
		pathsEast[i]  = false;
		pathsWest[i]  = false;
	}

	/*
            This maze generation works using a tree style method, where each branch or path is generated one at a time, and then
            any open entrances that is left is also then added to the currentPaths vector, ready to be generated in the next layer.
            TODO: Add the ability to go back, if not many rooms have been created and look for spaces some can be
        */

	std::vector<Vec2i> newPaths;    // This stores the newPaths for the next layer
	newPaths.reserve(BOARD_SIZE);   // Reserves space because, not all spaces may be used
	while(currentPaths.size() > 0)
	{
		for(int i = 0; i < currentPaths.size(); i++)
		{   // This goes through all the current path options and generates the options for the rooms
			Vec2i pos = currentPaths[i];

			if(get(pos.y, pos.x))   // Checks that the pointer is a nullptr - so it doesn't overwrite any rooms
				continue;

			int prob = startMax;   // This sets the probability of for the chance of generating an entrance
			if(pos.x > (3 * BOARD_SIZE / 4 + 1) || pos.x < BOARD_SIZE / 4 + 1)
			{
				prob++;
			}
			else if(pos.y > (3 * BOARD_SIZE / 4 + 1) || pos.y < BOARD_SIZE / 4 + 1)
			{
				prob++;
			}

			int pathCount = 0;   // Stores the number of paths from that room

			// 0: closed but could be open, 1: open, 2: closed and cannot be open
			EntranceState north = shouldBeOpen(get(pos.y + 1, pos.x), SOUTH_ENTRANCE, prob, &pathCount);
			EntranceState south = shouldBeOpen(get(pos.y - 1, pos.x), NORTH_ENTRANCE, prob, &pathCount);
			EntranceState east  = shouldBeOpen(get(pos.y, pos.x + 1), WEST_ENTRANCE, prob, &pathCount);
			EntranceState west  = shouldBeOpen(get(pos.y, pos.x - 1), EAST_ENTRANCE, prob, &pathCount);

			// To increase the spread for the beginning part of generation, if only one entrance has been generated -
			// then it will force another entrance if the layer is still below the layerMax
			if(pathCount <= 1 && layer < layerMax)
			{
				forceEntrance(&north, &south, &east, &west);

				int r = Random::getNum(0, 2);
				if(r != 2)
				{
					forceEntrance(&north, &south, &east, &west);
				}
			}
			else if(pathCount == 2 && layer < layerMax - BOARD_SIZE / 3)
			{
				int r = Random::getNum(0, 2);
				if(r != 2)
				{
					forceEntrance(&north, &south, &east, &west);
				}
			}

			// This is to check if any errors have occurred when generating the maze
			if(north == EntranceState::isOpen && pos.y < BOARD_SIZE - 1 && get(pos.y + 1, pos.x) && !get(pos.y + 1, pos.x)->isOpen(SOUTH_ENTRANCE))
				Log::error("Room generated incorrectly!", LOGINFO);
			if(south == EntranceState::isOpen && pos.y > 0 && get(pos.y - 1, pos.x) && !get(pos.y - 1, pos.x)->isOpen(NORTH_ENTRANCE))
				Log::error("Room generated incorrectly!", LOGINFO);
			if(east == EntranceState::isOpen && pos.x < BOARD_SIZE - 1 && get(pos.y, pos.x + 1) && !get(pos.y, pos.x + 1)->isOpen(WEST_ENTRANCE))
				Log::error("Room generated incorrectly!", LOGINFO);
			if(west == EntranceState::isOpen && pos.x > 0 && get(pos.y, pos.x - 1) && !get(pos.y, pos.x - 1)->isOpen(EAST_ENTRANCE))
				Log::error("Room generated incorrectly!", LOGINFO);

			// This adds any new paths made to the newPaths list
			if(north == EntranceState::isOpen && pos.y < BOARD_SIZE - 1 && !get(pos.y + 1, pos.x))
				newPaths.push_back({pos.x, pos.y + 1});
			if(south == EntranceState::isOpen && pos.y > 0 && !get(pos.y - 1, pos.x))
				newPaths.push_back({pos.x, pos.y - 1});
			if(east == EntranceState::isOpen && pos.x < BOARD_SIZE - 1 && !get(pos.y, pos.x + 1))
				newPaths.push_back({pos.x + 1, pos.y});
			if(west == EntranceState::isOpen && pos.x > 0 && !get(pos.y, pos.x - 1))
				newPaths.push_back({pos.x - 1, pos.y});

			addRoom(pos.x, pos.y,
					north == EntranceState::isOpen,
					south == EntranceState::isOpen,
					east == EntranceState::isOpen,
					west == EntranceState::isOpen);
		}
		currentPaths = newPaths;
		newPaths.clear();

		layer++;
	}

	updatePaths();

	finishedGenerating = true;
}

void Maze::multithreadGenerating(int layerMax, int startMax)
{
	if(!finishedGenerating)
		Log::critical("Stacked maze generating!!", LOGINFO);
	finishedGenerating = false;
	std::thread t1(&Maze::generatePaths, this, layerMax, startMax);   // This starts the multithreading
	t1.detach();
}

void Maze::generate()
{
	for(int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
	{
		if(board[i])
		{
			delete board[i];   // NOTE: This frees up the memory, but does not make it a nullptr
			board[i] = nullptr;
		}
	}
	yoffset      = 0;
	xoffset      = 0;
	int midpoint = BOARD_SIZE / 2;
	// NOTE: MUST DELETE ALL ROOMS!

	addRoom(midpoint, midpoint, true, true, true, true);
	currentPaths.push_back({midpoint - 1, midpoint});
	currentPaths.push_back({midpoint, midpoint - 1});
	currentPaths.push_back({midpoint + 1, midpoint});
	currentPaths.push_back({midpoint, midpoint + 1});

	multithreadGenerating(BOARD_SIZE * 4 / 5, 1);
	// TODO: Add check to see if okay maze
}

void Maze::moveNorth()
{
	for(int i = 0; i < BOARD_SIZE; i++)
	{
		// NOTE: The coordinates given is the top layer
		if(pathsNorth[i])
			currentPaths.push_back({i, BOARD_SIZE - 1});

		// This deletes any room that is being forgotten
		// NOTE it is 0 because it is getting rid of the bottom layer - which will become the top layer
		removeRoom(0, i);
	}
	yoffset++;   // Changes the y offset because of the new layout
	if(yoffset == BOARD_SIZE)
		yoffset = 0;
	if(currentPaths.size() > 0)   // This checks if there is any point to generate new paths
		multithreadGenerating(LAYER_MAX_FOR_DIRECTIONS, 1);
	else
		updatePaths();
}

void Maze::moveSouth()
{   // These all do the same as moveNorth but just specialized for the direction
	for(int i = 0; i < BOARD_SIZE; i++)
	{
		if(pathsSouth[i])
			currentPaths.push_back({i, 0});

		removeRoom(BOARD_SIZE - 1, i);
	}
	yoffset--;
	if(yoffset == -1)
		yoffset = BOARD_SIZE - 1;
	if(currentPaths.size() > 0)
		multithreadGenerating(LAYER_MAX_FOR_DIRECTIONS, 1);
	else
		updatePaths();
}

void Maze::moveEast()
{
	for(int i = 0; i < BOARD_SIZE; i++)
	{
		if(pathsEast[i])
			currentPaths.push_back({BOARD_SIZE - 1, i});

		removeRoom(i, 0);
	}
	xoffset++;
	if(xoffset == BOARD_SIZE)
		xoffset = 0;
	if(currentPaths.size() > 0)
		multithreadGenerating(LAYER_MAX_FOR_DIRECTIONS, 1);
	else
		updatePaths();
}

void Maze::moveWest()
{
	for(int i = 0; i < BOARD_SIZE; i++)
	{
		if(pathsWest[i])
			currentPaths.push_back({0, i});

		removeRoom(i, BOARD_SIZE - 1);
	}
	xoffset--;
	if(xoffset == -1)
		xoffset = BOARD_SIZE - 1;
	if(currentPaths.size() > 0)
		multithreadGenerating(LAYER_MAX_FOR_DIRECTIONS, 1);
	else
		updatePaths();
}

Maze::EntranceState Maze::shouldBeOpen(Room *room, int nextEntrance, int prob, int *pathCount)
{
	if(room)
	{
		if(room->isOpen(nextEntrance))
		{   // If the opposite entrance is open then it needs to have the entrance open
			(*pathCount)++;
			return EntranceState::isOpen;
		}
		else   // If it is closed then there cannot be an entrance
			return EntranceState::isClosed;
	}
	else
	{   // Randomly generates whether the entrance will be open
		int r = Random::getNum(0, prob);
		if(r == 0)
		{
			(*pathCount)++;
			return EntranceState::isOpen;
		}
		return EntranceState::couldOpen;
	}
}

void Maze::forceEntrance(Maze::EntranceState *north, Maze::EntranceState *south, Maze::EntranceState *east, Maze::EntranceState *west)
{
	std::vector<EntranceState *> entrances;   // This will store the pointers to the entrance values
	entrances.reserve(3);

	// This checks if another entrance can be made - if so it adds it to the list
	if(!(*north))
		entrances.push_back(north);
	if(!(*south))
		entrances.push_back(south);
	if(!(*east))
		entrances.push_back(east);
	if(!(*west))
		entrances.push_back(west);

	if(entrances.size() == 1)   // This just makes sure that random
	{
		*entrances[0] = EntranceState::isOpen;
	}
	else if(entrances.size() > 0)
	{
		int r         = Random::getNum(0, entrances.size() - 1);
		*entrances[r] = EntranceState::isOpen;
	}
}

void Maze::updatePaths()
{
	/* NOTE: Probably don't even try to change where this is calculated! This must go here, because if it doesn't you would have to do something special with the paths variables or realise that you actually have to check at
        some point if the player is stuck.*/

	for(int i = 0; i < BOARD_SIZE; i++)
	{
		if(get(BOARD_SIZE - 1, i) && get(BOARD_SIZE - 1, i)->isOpen(NORTH_ENTRANCE))
			pathsNorth[i] = true;
		if(get(0, i) && get(0, i)->isOpen(SOUTH_ENTRANCE))
			pathsSouth[i] = true;
		if(get(i, BOARD_SIZE - 1) && get(i, BOARD_SIZE - 1)->isOpen(EAST_ENTRANCE))
			pathsEast[i] = true;
		if(get(i, 0) && get(i, 0)->isOpen(WEST_ENTRANCE))
			pathsWest[i] = true;
	}
}

Entity *Maze::entityCollisionDetection(float nextX, float nextY, CollisionBox collisionBox)
{
	if(m_Player.hasCollidedWith(nextX, nextY, collisionBox))
		return &m_Player;
	return Level::entityCollisionDetection(nextX, nextY, collisionBox);
}
// !SECTION

// SECTION: Getters
Room *Maze::get(int y, int x)
{
	if(x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
		return nullptr;
	return board[coordsToIndex(x, y)];
}

Tile *Maze::getTile(int x, int y)
{
	int roomX = x / ROOM_SIZE;
	int tileX = x % ROOM_SIZE;
	int roomY = y / ROOM_SIZE;
	int tileY = y % ROOM_SIZE;

	Room *room = get(roomY, roomX);
	if(!room)
	{
		// Log::warning("Trying to access room that doesn't exist!");
		return nullptr;
	}

	return room->getTile(tileX, tileY);
}
// !SECTION
