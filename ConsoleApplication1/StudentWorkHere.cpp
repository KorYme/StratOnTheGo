#include "FlowFieldMap.h"
#include "PlayerController.h"
#include "Map.h"
#include "Game.h"
#include "Unit.h"

#include <queue>
#include <memory>
#include <cmath>
#include <algorithm>

void PlayerController::SelectNearestUnitUsingBFS()
{
	//This function selects the nearest non selected unit from 1st selected one using BFS (units are in selectedUnits)
	if (selectedUnits.empty())
		return;
	std::queue<CVect> visiting;
	CVect firstUnitPosition = (*selectedUnits.begin())->GetPosition();
	std::set<CVect> alreadyTreated;
	visiting.push(firstUnitPosition);
	alreadyTreated.insert(firstUnitPosition);
 	while (!visiting.empty())
	{
		CVect currentPos = visiting.front();
		std::shared_ptr<Unit> unit = game->GetUnitAtPos(currentPos.x, currentPos.y, true);
		if (unit != nullptr)
		{
			SelectUnit(unit);
			return;
		}
		for (int x = -1; x < 2; x++)
		{
			for (int y = -1; y < 2; y++)
			{
				if ((!allowDiag && x != 0 && y != 0) || (x == 0 && y == 0) || (game->GetMap()->GetTile(currentPos.x + x, currentPos.y + y) == nullptr))
				{
					continue;
				}
				CVect position = CVect(currentPos.x + x, currentPos.y + y, 0);
				if (alreadyTreated.find(position) == alreadyTreated.end())
				{
					visiting.push(position);
					alreadyTreated.insert(position);
				}
			}
		}
		visiting.pop();
	}
}

float ComputeHeuristic(CVect posA, CVect posB, bool allowDiags)
{
	if (allowDiags)
	{
		return std::max(abs(posA.x - posB.x), abs(posA.y - posB.y));
	}
	else
	{
		return abs(posA.x - posB.x) + abs(posA.y - posB.y);
	}
}

void PlayerController::ComputePathUsingAStar(std::shared_ptr<Unit> unit, CVect to, bool allowdiag = true)
{
	std::shared_ptr<Map> map = game->GetMap();
	if (map->GetTile(to.x, to.y)->GetCost() == UINT32_MAX)
	{
		return;
	}

	std::set<CVect> alreadyTreated;
	std::map<CVect, float> visitingPriority;

	std::map<CVect, CVect> closestToStartParent;
	std::map<CVect, float> costFromStart;

	CVect currentPos = unit->GetPosition();
	currentPos.x = (int)currentPos.x + 0.5f;
	currentPos.y = (int)currentPos.y + 0.5f;
	to.x = (int)to.x + 0.5f;
	to.y = (int)to.y + 0.5f;
	
	visitingPriority.emplace(currentPos, 0);
	costFromStart.emplace(currentPos, 0);

	while (!visitingPriority.empty())
	{
		std::map<CVect, float>::iterator prioIt = visitingPriority.begin();
		currentPos = prioIt->first;
		float minimumPrio = prioIt->second;
		prioIt++;
		while (prioIt != visitingPriority.end())
		{
			if (prioIt->second < minimumPrio)
			{
				minimumPrio = prioIt->second;
				currentPos = prioIt->first;
			}
			prioIt++;
		}
		visitingPriority.erase(currentPos);
		alreadyTreated.insert(currentPos);
		if (currentPos == to)
		{
			break;
		}
		for (int x = -1; x < 2; x++)
		{
			for (int y = -1; y < 2; y++)
			{
				if ((x == 0 && y == 0) || ((x != 0 && y != 0)
					&& (!allowDiag || map->GetTileCost(currentPos.x, currentPos.y + y) == UINT32_MAX
					|| map->GetTileCost(currentPos.x + x, currentPos.y) == UINT32_MAX)))
				{
					continue;
				}
				CVect tilePos = CVect(currentPos.x + x, currentPos.y + y, 0);
				std::shared_ptr<Tile> tile = map->GetTile(tilePos.x, tilePos.y);
				if (tile == nullptr)
				{
					continue;
				}
				if (alreadyTreated.find(tilePos) != alreadyTreated.end())
				{
					continue;
				}
				if (tile->GetCost() == UINT32_MAX)
				{
					alreadyTreated.insert(tilePos);
					continue;
				}
				float tileCost = costFromStart.find(currentPos)->second + tile->GetCost() / 2.f + map->GetTileCost(currentPos.x, currentPos.y) / 2.f;
				std::map<CVect, CVect>::iterator lastParentIterator = closestToStartParent.find(tilePos);
				if (lastParentIterator == closestToStartParent.end())
				{
					closestToStartParent.emplace(tilePos, currentPos);
					costFromStart.emplace(tilePos, tileCost);
					visitingPriority.emplace(tilePos, tileCost + ComputeHeuristic(tilePos, to, allowdiag));
				}
				else if (costFromStart.find(lastParentIterator->second)->second + map->GetTileCost(lastParentIterator->second.x, lastParentIterator->second.y) / 2.f
					> costFromStart.find(currentPos)->second + map->GetTileCost(currentPos.x, currentPos.y) / 2.f)
				{
					closestToStartParent[tilePos] = currentPos;
					costFromStart[tilePos] = tileCost;
					visitingPriority[tilePos] = tileCost + ComputeHeuristic(tilePos, to, allowdiag);
				}
			}
		}
	}

	if (closestToStartParent.find(to) == closestToStartParent.end())
	{
		return;
	}

	std::vector<CVect> path;
	CVect firstUnitPosition = CVect((int)unit->GetPosition().x + .5f, (int)unit->GetPosition().y + .5f, 0);
	while (to != firstUnitPosition)
	{
		path.push_back(to);
		to = closestToStartParent[to];
	}
	std::reverse(path.begin(), path.end());
	unit->SetPath(path);
}

void FlowFieldMap::GenerateFlowField(std::shared_ptr<Map> sourceMap, CVect dest, bool allowDiags)
{
	ResetMap(sourceMap->GetWidth(), sourceMap->GetHeight());
	if (sourceMap->GetTile(dest.x, dest.y)->GetCost() == UINT32_MAX)
	{
		return;
	}
	std::set<CVect> visiting;
	std::set<CVect> alreadyTreated;
	CVect currentPos = CVect((int)dest.x, (int)dest.y, 0);
	visiting.insert(currentPos);
	distanceMap[dest.y][dest.x] = 0;
	while (!visiting.empty())
	{
		std::set<CVect>::iterator it = visiting.begin();
		unsigned int minimumCost = distanceMap[it->y][it->x];
		currentPos = *it;
		it++;
		while (it != visiting.end())
		{
			if (distanceMap[it->y][it->x] < minimumCost)
			{
				currentPos = CVect(it->x, it->y, 0);
				minimumCost = distanceMap[it->y][it->x];
			}
			it++;
		}
		alreadyTreated.insert(currentPos);
		visiting.erase(currentPos);
		for (int x = -1; x < 2; x++)
		{
			for (int y = -1; y < 2; y++)
			{
				if ((x == 0 && y == 0) || ((x != 0 && y != 0)
					&& (!allowDiags || sourceMap->GetTileCost(currentPos.x, currentPos.y + y) == UINT32_MAX
					|| sourceMap->GetTileCost(currentPos.x + x, currentPos.y) == UINT32_MAX)))
				{
					continue;
				}
				CVect tilePos = CVect(currentPos.x + x, currentPos.y + y, 0);
				if (alreadyTreated.find(tilePos) != alreadyTreated.end())
				{
					continue;
				}
				std::shared_ptr<Tile> tile = sourceMap->GetTile(tilePos.x, tilePos.y);
				if (tile == nullptr)
				{
					continue;
				}
				if (tile->GetCost() == UINT32_MAX)
				{
					alreadyTreated.insert(tilePos);
					continue;
				}
				int tileCost = tile->GetCost() + distanceMap[currentPos.y][currentPos.x];
				if (distanceMap[tilePos.y][tilePos.x] > tileCost
					|| (distanceMap[tilePos.y][tilePos.x] == tileCost && flowField[tilePos.y][tilePos.x].length2() > (currentPos - tilePos).length2()))
				{
					distanceMap[tilePos.y][tilePos.x] = tile->GetCost() + distanceMap[currentPos.y][currentPos.x];
					flowField[tilePos.y][tilePos.x] = currentPos - tilePos;
					if (visiting.find(tilePos) == visiting.end())
					{
						visiting.insert(tilePos);
					}
				}
			}
		}
	}
}