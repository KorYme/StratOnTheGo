#pragma once
#include <SDL_render.h>
#include <set>
#include <memory>
#include "CVect.h"

class Unit;
class Game;
class FlowFieldMap;

enum class MoveMode
{
	simple,
	path,
	aStar,
	FlowField
};

const float TWO_SQRT = 1.41421356237f;

class PlayerController
{
protected:
	//selection
	bool selectionInProgress;
	SDL_Rect selectionRect;
	std::set< std::shared_ptr<Unit>> selectedUnits;
	SDL_Texture* textTexture = nullptr;
	bool textTextureDirty = true;
	MoveMode moveMode = MoveMode::simple;
	bool allowDiag = false;


	std::shared_ptr<FlowFieldMap> testMap;

	void UnselectAllUnits();
	void SelectUnitAtMousePos(int x, int y);

	void SelectUnitsInRect(const SDL_Rect& rect);
	std::shared_ptr<Game> game;

	void SelectNearestUnitUsingBFS();
	void ComputePathUsingAStar(std::shared_ptr<Unit> unit, CVect to, bool allowdiag);
	void RebuildText(SDL_Renderer* screenRenderer);
public:
	void Init(std::shared_ptr<Game> game);
	void Update(int deltaTime);

	void Render(SDL_Renderer* screenRenderer);

	void SelectUnit(std::shared_ptr<Unit> unit);

	PlayerController() = default;
	~PlayerController();
};

