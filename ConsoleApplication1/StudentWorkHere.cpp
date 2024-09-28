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

typedef struct TilePriority
{
	int priority;
	CVect position;

	TilePriority(int newPrio, CVect newPosition) 
	{
		priority = newPrio;
		position = newPosition;
	}
};

void PlayerController::ComputePathUsingAStar(std::shared_ptr<Unit> unit, CVect to, bool allowdiag = true)
{
	// Check if the tile clicked is an accessible tile
	if (game->GetMap()->GetTile(to.x, to.y)->GetCost() == UINT32_MAX)
	{
		return;
	}
	std::list<TilePriority> visiting;
	std::set<CVect> alreadyTreated;
	std::map<CVect, CVect> closestToStartParent;
	std::map<CVect, int> costFromStart;

	CVect currentPos = unit->GetPosition();
	currentPos.x = (int)currentPos.x + 0.5f;
	currentPos.y = (int)currentPos.y + 0.5f;
	to.x = (int)to.x + 0.5f;
	to.y = (int)to.y + 0.5f;
	CVect firstUnitPosition(currentPos);
	visiting.push_back(TilePriority(0, currentPos));
	costFromStart.emplace(currentPos, 0);

	while (!visiting.empty())
	{
		currentPos = visiting.front().position;
		visiting.pop_front();
		if (currentPos == to)
		{
			break;
		}
		if (alreadyTreated.find(currentPos) != alreadyTreated.end())
		{
			continue;
		}
		for (int x = -1; x < 2; x++)
		{
			for (int y = -1; y < 2; y++)
			{
				if ((!allowDiag && x != 0 && y != 0) || (x == 0 && y == 0))
				{
					continue;
				}
				std::shared_ptr<Tile> tile = game->GetMap()->GetTile(currentPos.x + x, currentPos.y + y);
				if (tile == nullptr || tile->GetCost() == UINT32_MAX)
				{
					continue;
				}
				CVect tilePos = CVect(currentPos.x + x, currentPos.y + y, 0);
				int tileCost = costFromStart.find(currentPos)->second + game->GetMap()->GetTileCost(tilePos.x, tilePos.y);
				std::map<CVect, CVect>::iterator lastParentIterator = closestToStartParent.find(tilePos);
				if (lastParentIterator == closestToStartParent.end())
				{
					closestToStartParent.emplace(tilePos, currentPos);
					costFromStart.emplace(tilePos, tileCost);
					std::list<TilePriority>::iterator it = visiting.begin();
					int priority = tileCost + ComputeHeuristic(tilePos, to);
					while (it != visiting.end())
					{
						if (it->priority > priority)
						{
							visiting.insert(it, TilePriority(priority, tilePos));
							it = visiting.begin();
							break;
						}
						it++;
					}
					if (it == visiting.end())
					{
						visiting.push_back(TilePriority(priority, tilePos));
					}
				}
				else
				{
					if (costFromStart.find(lastParentIterator->second)->second > costFromStart.find(currentPos)->second)
					{
						closestToStartParent[tilePos] = currentPos;
						costFromStart[tilePos] = tileCost;
						std::list<TilePriority>::iterator it = visiting.begin();
						int priority = tileCost + ComputeHeuristic(tilePos, to);
						while (it != visiting.end())
						{
							if (it->priority > priority)
							{
								visiting.insert(it, TilePriority(priority, tilePos));
								it = visiting.begin();
								break;
							}
							it++;
						}
						if (it == visiting.end())
						{
							visiting.push_back(TilePriority(priority, tilePos));
						}
					}
				}
			}
		}
		alreadyTreated.insert(currentPos);
	}

	if (closestToStartParent.find(to) == closestToStartParent.end())
	{
		return;
	}

	std::vector<CVect> path;
	while (to != firstUnitPosition)
	{
		path.push_back(to);
		to = closestToStartParent[to];
	}
	std::reverse(path.begin(), path.end());
	unit->SetPath(path);
}

float PlayerController::ComputeHeuristic(CVect posA, CVect posB) 
{
	float dx = abs(posA.x - posB.x);
	float dy = abs(posA.y - posB.y);
	return dx + dy + (TWO_SQRT - 2) * fmin(dx, dy);
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
		alreadyTreated.insert(currentPos);
		for (int x = -1; x < 2; x++)
		{
			for (int y = -1; y < 2; y++)
			{
				if ((!allowDiags && x != 0 && y != 0) || (x == 0 && y == 0))
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
				if (distanceMap[tilePos.y][tilePos.x] > tile->GetCost() + distanceMap[currentPos.y][currentPos.x])
				{
					distanceMap[tilePos.y][tilePos.x] = tile->GetCost() + distanceMap[currentPos.y][currentPos.x];
					flowField[tilePos.y][tilePos.x] = currentPos - tilePos;
					flowField[tilePos.y][tilePos.x].normalize();
					if (visiting.find(tilePos) == visiting.end())
					{
						visiting.insert(tilePos);
					}
				}
			}
		}
		visiting.erase(currentPos);
		unsigned int minimumCost = 0xFFFFFFFF;
		std::set<CVect>::iterator it = visiting.begin();
		while (it != visiting.end())
		{
			if (distanceMap[it->y][it->x] < minimumCost)
			{
				currentPos = CVect(it->x, it->y, 0);
				minimumCost = distanceMap[it->y][it->x];
			}
			it++;
		}
	}
}