#include "./headers/SquareCellGrid.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <random>
#include <math.h>

#include "./headers/RandomNumberGenerators.h"
#include "./headers/MathConstants.h"

using namespace std;

const double BOLTZ_TEMP = 10.0f;

//Volume Constraint Strength
const double LAMBDA = 5.0f;

//Surface constraint strength
const double SIGMA = 0.0f;

const auto J = CellTypes::J;

SquareCellGrid::SquareCellGrid(int w, int h) : internalGrid(w + 2, std::vector<Cell>(h + 2)), pixels((w+2) * (h+2) * 4, 0) {

	interiorWidth = w;
	interiorHeight = h;
	boundaryWidth = w + 2;
	boundaryHeight = h + 2;

	for (int x = 0; x < boundaryWidth; x++) {
		internalGrid[x][0] = Cell((int)CELL_TYPE::BOUNDARY);
		internalGrid[x][boundaryHeight - 1] = Cell((int)CELL_TYPE::BOUNDARY);
	};

	for (int y = 0; y < boundaryHeight; y++) {
		internalGrid[0][y] = Cell((int)CELL_TYPE::BOUNDARY);
		internalGrid[boundaryWidth - 1][y] = Cell((int)CELL_TYPE::BOUNDARY);
	};
	
	SuperCell::setVolume(1, interiorWidth * interiorHeight);
	SuperCell::setVolume(0, (boundaryWidth*boundaryHeight) - (interiorWidth * interiorHeight));

}

vector<Cell*> SquareCellGrid::getNeighbours(int row, int col)
{

	vector<Cell*> neighbours;

	neighbours.reserve(8);

	for (int x = -1; x <= 1; x++) {

		for (int y = -1; y <= 1; y++) {

			if (x == 0 && y == 0) continue;
			neighbours.push_back(&(internalGrid[row + x][col + y]));

		}

	}

	return neighbours;

}

std::vector<Cell*> SquareCellGrid::getNeighbours(int row, int col, CELL_TYPE t) {
	
	vector<Cell*> neighbours = getNeighbours(row, col);

	neighbours.erase(std::remove_if(
		neighbours.begin(),
		neighbours.end(),
		[t](const Cell* c) { return c->getType() != t; })
		, neighbours.end()
	);

	return neighbours;

}

vector<Vector2D<int>> SquareCellGrid::getNeighboursCoords(int row, int col)
{
	vector<Vector2D<int>> neighbours;

	neighbours.reserve(8);

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {

			if (x == 0 && y == 0) continue;
			neighbours.push_back(Vector2D<int>(row + x, col + y));

		}
	}
	return neighbours;
}

std::vector<Vector2D<int>> SquareCellGrid::getNeighboursCoords(int row, int col, CELL_TYPE t) {
	
	vector<Vector2D<int>> neighbours;

	neighbours.reserve(8);

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {

			if (x == 0 && y == 0) continue;
			if (internalGrid[row - 1][col - 1].getType() == t) neighbours.push_back(Vector2D<int>(row + x, col + y));
			
		}
	}
	return neighbours;

}

int SquareCellGrid::divideCell(int c) {

	int minX = interiorWidth;
	int minY = interiorHeight;
	int maxX = 1;
	int maxY = 1;

	vector<Vector2D<int>> cellList;
	vector<Vector2D<int>> newList;

	for (int X = 1; X <= interiorWidth; X++) {
		for (int Y = 1; Y <= interiorHeight; Y++) {

			if (internalGrid[X][Y].getSuperCell() == c) {

				cellList.push_back(Vector2D<int>(X, Y));

				if (X < minX) minX = X;
				if (X > maxX) maxX = X;
				if (Y < minY) minY = Y;
				if (Y > maxY) maxY = Y;

			}

		}
	}

	if (cellList.size() <= 1) {
		return 1;
	}

	int splitAxis;
	int midPoint;

	if ((maxX - minX) > (maxY - minY)) {
		splitAxis = 0;
		midPoint = (maxX + minX) / 2;
	}
	else {
		splitAxis = 1;
		midPoint = (maxY + minY) / 2;
	}

	for (unsigned int k = 0; k < cellList.size(); k++) {
		if (cellList[k][splitAxis] < midPoint) {
			newList.push_back(cellList[k]);
		}
	}

	SuperCell::increaseGeneration(c);
	int newSuperCell = SuperCell::makeNewSuperCell(c);

	SuperCell::setMCS(c, 0);
	SuperCell::setMCS(newSuperCell, 0);

	for (unsigned int c = 0; c < newList.size(); c++) {
		Vector2D<int>& V = newList[c];
		setCell(V[0], V[1], newSuperCell);
	}

	return newSuperCell;

}

int SquareCellGrid::divideCellRandomAxis(int c) {

	int minX = interiorWidth;
	int minY = interiorHeight;
	int maxX = 1;
	int maxY = 1;

	vector<Vector2D<int>> cellList;
	vector<Vector2D<int>> newList;

	for (int X = 1; X <= interiorWidth; X++) {
		for (int Y = 1; Y <= interiorHeight; Y++) {

			if (internalGrid[X][Y].getSuperCell() == c) {

				cellList.push_back(Vector2D<int>(X, Y));

				if (X < minX) minX = X;
				if (X > maxX) maxX = X;
				if (Y < minY) minY = Y;
				if (Y > maxY) maxY = Y;

			}

		}
	}

	if (cellList.size() <= 1) {
		return -1;
	}

	int midX = (int)(0.5 * (minX + maxX));
	int midY = (int)(0.5 * (minY + maxY));

	int gradM = RandomNumberGenerators::rUnifInt(-89, 89);
	double grad = tan(gradM*PI_D/180.f);

	for (unsigned int k = 0; k < cellList.size(); k++) {
		if (cellList[k][1] > grad*(cellList[k][0]-midX)+midY) {
			newList.push_back(cellList[k]);
		}
	}

	SuperCell::increaseGeneration(c);
	int newSuperCell = SuperCell::makeNewSuperCell(c);

	SuperCell::setMCS(c, 0);
	SuperCell::setMCS(newSuperCell, 0);

	for (unsigned int c = 0; c < newList.size(); c++) {
		Vector2D<int>& V = newList[c];
		setCell(V[0], V[1], newSuperCell);
	}

	return newSuperCell;

}

int SquareCellGrid::divideCellShortAxis(int c) {

	vector<Vector2D<int>> cellList;
	vector<Vector2D<int>> newList;

	//Find all subcells in cell
	for (int X = 1; X <= interiorWidth; X++) {
		for (int Y = 1; Y <= interiorHeight; Y++) {

			if (internalGrid[X][Y].getSuperCell() == c) {

				cellList.push_back(Vector2D<int>(X, Y));

			}

		}

	}
	
	//Abort if cell less than one subcell
	if (cellList.size() <= 1) {
		return -1;
	}

	//Calculate short axis of cell
	double m00 = calculateRawImageMoment(cellList, 0, 0);
	double m10 = calculateRawImageMoment(cellList, 1, 0);
	double m01 = calculateRawImageMoment(cellList, 0, 1);

	double xBar = m10 / m00;
	double yBar = m01 / m00;

	double mu20 = (calculateRawImageMoment(cellList, 2, 0) / m00) - pow(xBar, 2);
	double mu02 = (calculateRawImageMoment(cellList, 0, 2) / m00) - pow(yBar, 2);
	double mu11 = (calculateRawImageMoment(cellList, 1, 1) / m00) - xBar*yBar;

	double covTrace = mu20 + mu02;
	double covDet = mu20 * mu02 - pow(mu11,2);

	double eigA = (- covTrace + sqrt(pow(covTrace, 2) - 4 * covDet)) / 2;
	double eigB = (- covTrace - sqrt(pow(covTrace, 2) - 4 * covDet)) / 2;

	double smallEig = min(abs(eigA), abs(eigB));

	Vector2D<double> eigVec(mu11, smallEig-mu20);
	double grad = eigVec[1] / eigVec[0];

	for (unsigned int k = 0; k < cellList.size(); k++) {
		if (cellList[k][1] > grad * (cellList[k][0] - xBar) + yBar) {
			newList.push_back(cellList[k]);
		}
	}

	SuperCell::increaseGeneration(c);
	int newSuperCell = SuperCell::makeNewSuperCell(c);

	SuperCell::setMCS(c, 0);
	SuperCell::setMCS(newSuperCell, 0);

	for (unsigned int c = 0; c < newList.size(); c++) {
		Vector2D<int>& V = newList[c];
		setCell(V[0], V[1], newSuperCell);
	}

	return newSuperCell;

}

double SquareCellGrid::calculateRawImageMoment(std::vector<Vector2D<int>> data, int iO, int jO) {
	
	double moment = 0.0f;
	
	for (Vector2D<int> V : data) {

		moment += pow(V[0], iO) * pow(V[1], jO);

	}

	return moment;
}

int SquareCellGrid::cleaveCell(int c) {

	int superCellA = c;
	int superCellB = divideCellShortAxis(c);

	if (superCellB == -1) return -1;

	int newTargetVolume = SuperCell::getTargetVolume(c) / 2;
	int newTargetSurface = (int) sqrt(newTargetVolume) * BORDER_CONST;

	SuperCell::setTargetVolume(superCellA, newTargetVolume);
	SuperCell::setTargetVolume(superCellB, newTargetVolume);

	SuperCell::setTargetSurface(superCellA, newTargetSurface);
	SuperCell::setTargetSurface(superCellB, newTargetSurface);

	return superCellB;
}

int SquareCellGrid::moveCell(int x, int y) {

	vector<Vector2D<int>> neighbours = getNeighboursCoords(x, y);

	int r = RandomNumberGenerators::rUnifInt(0, (int)neighbours.size() - 1);

	int targetX = neighbours[r][0];
	int targetY = neighbours[r][1];

	Cell& origin = internalGrid[x][y];
	Cell& swap = internalGrid[neighbours[r][0]][neighbours[r][1]];

	if (swap.getType() != CELL_TYPE::BOUNDARY &&
		swap.getSuperCell() != internalGrid[x][y].getSuperCell()) {

		double deltaH = getAdhesionDelta(x, y, targetX, targetY) + getVolumeDelta(x, y, targetX, targetY);

		if (deltaH <= 0 || (RandomNumberGenerators::rUnifProb() < exp(-deltaH / BOLTZ_TEMP))) {
			setCell(targetX, targetY, internalGrid[x][y].getSuperCell());
			
			localTextureRefresh(x, y);
			localTextureRefresh(targetX, targetY);

			return 1;	
		}

	}

	return 0;

}

int SquareCellGrid::calcSubCellPerimeter(int x, int y) {
	
	int activeSuper = internalGrid[x][y].getSuperCell();
	int perimeterCount = 0;

	if (internalGrid[x][y - 1].getSuperCell() != activeSuper) perimeterCount++;
	if (internalGrid[x][y + 1].getSuperCell() != activeSuper) perimeterCount++;
	if (internalGrid[x-1][y].getSuperCell() != activeSuper) perimeterCount++;
	if (internalGrid[x+1][y].getSuperCell() != activeSuper) perimeterCount++;

	return perimeterCount;

}

void SquareCellGrid::fullPerimeterRefresh() {

	for (int s = 0; s < SuperCell::getCounter(); s++) {
		SuperCell::setSurface(s, 0);
	}

	for (int x = 1; x <= interiorWidth; x++) {
		for (int y = 1; y < interiorHeight; y++) {

			int activeSuper = internalGrid[x][y].getSuperCell();

			SuperCell::changeSurface(activeSuper, calcSubCellPerimeter(x, y));

		}
	}

}

Cell& SquareCellGrid::getCell(int row, int col) {
	return internalGrid[row][col];
}

void SquareCellGrid::setCell(int x, int y, int superCell) {

	int originalSuper = internalGrid[x][y].getSuperCell();

	//Volume Change
	SuperCell::changeVolume(originalSuper, -1);
	SuperCell::changeVolume(superCell, 1);

	internalGrid[x][y].setSuperCell(superCell);

}

double SquareCellGrid::getAdhesionDelta(int sourceX, int sourceY, int destX, int destY) {

	Cell& source = internalGrid[sourceX][sourceY];
	Cell& dest = internalGrid[destX][destY];

	int sourceSuper = source.getSuperCell();
	int destSuper = dest.getSuperCell();

	double initH = 0.0f;
	double postH = 0.0f;

	vector<Cell*> neighbours = getNeighbours(destX, destY);
	for (int i = 0; i < 8; i++) {

		int nSuper = neighbours[i]->getSuperCell();

		initH += J[(int)dest.getType()][(int)neighbours[i]->getType()] * (nSuper != destSuper);
		postH += J[(int)source.getType()][(int)neighbours[i]->getType()] * (nSuper != sourceSuper);

	}

	return (postH - initH);
}

double SquareCellGrid::getVolumeDelta(int sourceX, int sourceY, int destX, int destY) {

	int destSuper = internalGrid[destX][destY].getSuperCell();

	//Prevent destruction of cells
	if (SuperCell::getVolume(destSuper) - 1 == 0) return 1000000.0f;

	int sourceSuper = internalGrid[sourceX][sourceY].getSuperCell();

	int sourceVol = SuperCell::getVolume(sourceSuper);
	int destVol = SuperCell::getVolume(destSuper);

	int sourceTarget = SuperCell::getTargetVolume(sourceSuper);
	int destTarget = SuperCell::getTargetVolume(destSuper);

	double deltaH = 0.0f;

	//Prevent medium volume from affecting energy
	bool sourceIgnore = sourceSuper == (int)CELL_TYPE::EMPTYSPACE;
	bool destIgnore = destSuper == (int)CELL_TYPE::EMPTYSPACE;

	deltaH = (!sourceIgnore)*((double)pow(sourceVol + 1 - sourceTarget, 2) - (double)pow(sourceVol - sourceTarget, 2))
			+ (!destIgnore)*((double)pow(destVol - 1 - destTarget, 2) - (double)pow(destVol - destTarget, 2));

	deltaH *= LAMBDA;

	return deltaH;
}

void SquareCellGrid::fullTextureRefresh() {

	for (int x = 0; x < boundaryWidth; x++) {

		for (int y = 0; y < boundaryHeight; y++) {

			const unsigned int pixOffset = (boundaryWidth * 4 * y) + x * 4;
			vector<int> colourIn = internalGrid[x][y].getColour();

			if (colourIn.size() == 0) {
				colourIn = { 0,0,0,0 };
			}

			pixels[pixOffset + 3] = (char) colourIn[0];
			pixels[pixOffset + 2] = (char) colourIn[1];
			pixels[pixOffset + 1] = (char) colourIn[2];
			pixels[pixOffset + 0] = SDL_ALPHA_OPAQUE;

		}

	}

}

void SquareCellGrid::localTextureRefresh(int x, int y) {

	const unsigned int pixOffset = (boundaryWidth * 4 * y) + x * 4;
	vector<int> colourIn = internalGrid[x][y].getColour();

	if (colourIn.size() == 0) {
		colourIn = { 0,0,0,0 };
	}

	pixels[pixOffset + 3] = (char)colourIn[0];
	pixels[pixOffset + 2] = (char)colourIn[1];
	pixels[pixOffset + 1] = (char)colourIn[2];
	pixels[pixOffset + 0] = SDL_ALPHA_OPAQUE;

}

std::vector<unsigned char> SquareCellGrid::getPixels() {

	return pixels;

}
