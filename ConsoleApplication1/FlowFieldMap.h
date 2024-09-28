#pragma once
#include <SDL_render.h>
#include <memory>
#include "CVect.h"
#include <vector>

class Map;

class FlowFieldMap
{
	std::vector<std::vector<CVect> > flowField;
	std::vector<std::vector<unsigned int> > distanceMap;

	void ResetMap(int width, int height);

	public:
	void GenerateFlowField(std::shared_ptr<Map> sourceMap, CVect dest, bool allowDiags);
	CVect GetNextTile(CVect current) const;

	void Render(SDL_Renderer* screenRenderer);

};

