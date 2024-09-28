#pragma once
#include "IGameEntity.h"
#include "Tile.h"
#include <map>
#include "SpriteFactory.h"


#define TILE_SIZE 32

class Map :
	public IGameEntity
{
protected:
	inline static std::map<TileType, std::shared_ptr<TileConfig>> tileConfigs
	{
		{ TileType::Grass, std::make_shared<TileConfig>(TileType::Grass, 1, 7,0,TILE_SIZE,TILE_SIZE)},
		{ TileType::HighGrass, std::make_shared<TileConfig>(TileType::HighGrass, 2, 4,10,TILE_SIZE,TILE_SIZE) },
		{ TileType::Dirt, std::make_shared<TileConfig>(TileType::Dirt, 4, 7,2,TILE_SIZE,TILE_SIZE) },
		{ TileType::Destroyed, std::make_shared<TileConfig>(TileType::Destroyed, 0Xffffffff , 12,0,TILE_SIZE,TILE_SIZE) }
	};

	std::weak_ptr<Map> thisWeakPtr;

	std::shared_ptr<Tile>** tileset = nullptr;
	std::shared_ptr<Sprite> spriteSheet = nullptr;

	int width;
	int height;

public:
	Map() = default;
	virtual ~Map();
	Map(const Map&) = delete;
	Map(Map&&) = delete;
	Map& operator=(const Map&) = delete;

	static std::shared_ptr<Map> Create();

	virtual std::shared_ptr<IGameEntity> GetSharedPtr();

	virtual void Init(const IGameEntityConfig* config);

	virtual void Release();

	virtual void Update(int deltaTime);

	virtual void Render(SDL_Renderer* screenRenderer);

	unsigned int GetTileCost(int x, int y);

	inline int GetWidth() { return width; }
	inline int GetHeight() { return height; }
	inline std::shared_ptr<Tile> GetTile(int x, int y) {
		if (x < 0 || x >= width || y < 0 || y >= height)
			return nullptr;
		return tileset[y][x]; 
	}
};

