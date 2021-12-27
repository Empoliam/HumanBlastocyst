#include <random>
#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>
#include <thread>
#include <math.h>

#define _USE_MATH_DEFINES
#include <cmath>

#include <boost/program_options.hpp>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "./headers/Cell.h"
#include "./headers/SquareCellGrid.h"
#include "./headers/Vector2D.h"
#include "./headers/RandomNumberGenerators.h"

unsigned int PIXEL_SCALE;
unsigned int MAX_MCS;
unsigned int SIM_WIDTH;
unsigned int SIM_HEIGHT;
unsigned int SIM_DELAY;
unsigned int RENDER_FPS;

//Number of MCS per real hour
const float MCS_HOUR_EST = 500.0f;

const int TARGET_INIT_CELLS = 1600;

//Target time morula cell division spacing
const float MCS_M_DIV_TARGET = 12*MCS_HOUR_EST;
const float SD_M_DIV_TARGET = 0.5*MCS_HOUR_EST;

//Target time of compaction
const float MCS_COMPACT_TARGET = 3 * 24 * MCS_HOUR_EST;
const float SD_COMPACT_TARGET = 0.5 * MCS_HOUR_EST;

//Target time of intiial differentiation
const float MCS_DIFFERENTIATE_TARGET = 4 * 24 * MCS_HOUR_EST;
const float SD_DIFFERENTIATE_TARGET = 1 * MCS_HOUR_EST;

//Fluid cell growth parameters
const int TARGET_MAX_FLUID = 6400;
const float TARGET_SCALE_FLUID = 36 * MCS_HOUR_EST;

//Trophectoderm division
const float MCS_T_DIV_TARGET_INIT = 9 * MCS_HOUR_EST;
const float MCS_T_DIV_SCALE_TIME = 250;
const float SD_T_DIV_TARGET = 3 * MCS_HOUR_EST;

int inline funcTrophectoderm(int m) {
	return MCS_T_DIV_TARGET_INIT + pow(m / MCS_T_DIV_SCALE_TIME, 2);
}

//ICM division parameters
const float MCS_I_DIV_TARGET = 12 * MCS_HOUR_EST;
const float SD_I_DIV_TARGET = 1 * MCS_HOUR_EST;

namespace po = boost::program_options;
using namespace std;

int simLoop(SquareCellGrid& grid, atomic<bool>& done);
int simInit(int argc, char* argv[]);
int printGrid(SDL_Renderer* renderer, SDL_Texture* texture, SquareCellGrid& grid);

int main(int argc, char* argv[]) {

	if (simInit(argc, argv) > 0) {
		return 1;
	}

	SDL_SetMainReady();
	SDL_Event event;
	SDL_Renderer* renderer;
	SDL_Window* window;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer((SIM_WIDTH + 2) * PIXEL_SCALE, (SIM_HEIGHT + 2) * PIXEL_SCALE, 0, &window, &renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	SuperCell::makeNewSuperCell(CELL_TYPE::BOUNDARY, 0, 0, 0);
	SuperCell::setColour((int)CELL_TYPE::BOUNDARY, 255, 255, 255, 255);
	SuperCell::makeNewSuperCell(CELL_TYPE::EMPTYSPACE, 0, 0, 0);
	SuperCell::setColour((int)CELL_TYPE::EMPTYSPACE, 0, 0, 0, 255);
	SuperCell::makeNewSuperCell(CELL_TYPE::FLUID, 0, 0, 50);
	SuperCell::setColour((int)CELL_TYPE::FLUID, 50, 50, 50, 255);

	SquareCellGrid grid(SIM_WIDTH, SIM_HEIGHT);
	
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, grid.boundaryWidth, grid.boundaryHeight);
	printGrid(renderer, texture, grid);

	std::atomic<bool> done(false);
	std::thread simLoopThread(simLoop, std::ref(grid), std::ref(done));
	
	bool quit = false;
	bool waitForEnd = false;

	unsigned int tickA, tickB, tickDelta;
	tickB = SDL_GetTicks();

	while (!quit) {

		if (!waitForEnd) {

			tickA = SDL_GetTicks();
			tickDelta = tickA - tickB;

			if (done) {				
				cout << "done";
				printGrid(renderer, texture, grid);
				simLoopThread.join();
				waitForEnd = true;
			}
			else {

				if (tickDelta > 1000 / RENDER_FPS) {

					//cout << "fps: " << (float)1000 / tickDelta << "\n";
					tickB = tickA;
					printGrid(renderer, texture, grid);

				}

			}

		}

		if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
			done = true;
			quit = true;
		}

	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

int simLoop(SquareCellGrid& grid, atomic<bool>& done) {
	
	//Target times for key events
	bool compacted = false;
	unsigned int compactionTime = (unsigned int)RandomNumberGenerators::rNormalFloat(MCS_COMPACT_TARGET, SD_COMPACT_TARGET);

	bool differentationA = false;
	unsigned int differentiationTime = (unsigned int)RandomNumberGenerators::rNormalFloat(MCS_DIFFERENTIATE_TARGET, SD_DIFFERENTIATE_TARGET);

	unsigned int diffStartMCS;

	//Initial cell setup
	int midX = SIM_WIDTH / 2;
	int midY = SIM_HEIGHT / 2;

	int targetInitCellsSqrt = (int)sqrt(TARGET_INIT_CELLS);
	int targetInitCellLength = (int) (2 * sqrt(3.14159 * TARGET_INIT_CELLS)) * 4;

	int newSuperCell = SuperCell::makeNewSuperCell(CELL_TYPE::GENERIC, 0, TARGET_INIT_CELLS, targetInitCellLength);

	SuperCell::setNextDiv(newSuperCell, (int) RandomNumberGenerators::rNormalFloat(MCS_M_DIV_TARGET,SD_M_DIV_TARGET));
	

	for (int x = midX - targetInitCellsSqrt / 2; x < midX + targetInitCellsSqrt / 2; x++) {
		for (int y = midY - targetInitCellsSqrt / 2; y < midY + targetInitCellsSqrt / 2; y++) {
		
			grid.setCell(x, y, newSuperCell);

		}
	}
	
	grid.fullTextureRefresh();

	//Number of samples to take before increasing MCS count
	unsigned int iMCS = grid.interiorWidth * grid.interiorHeight;

	//Simulation loop
	for (unsigned int m = 0; m < MAX_MCS; m++) {

		//Monte Carlo Step
		for (unsigned int i = 0; i < iMCS; i++) {

			int x = RandomNumberGenerators::rUnifInt(1, grid.interiorWidth);
			int y = RandomNumberGenerators::rUnifInt(1, grid.interiorHeight);

			bool success = grid.moveCell(x, y);

			if (done) {
				break;
			}

		}

		//Morula division stage
		if (!compacted) {
			for (int c = (int)CELL_TYPE::GENERIC; c < SuperCell::getCounter(); c++) {

				if (SuperCell::getCellType(c) == CELL_TYPE::GENERIC && SuperCell::getGeneration(c) < 4) {

					if (SuperCell::getMCS(c) >= SuperCell::getNextDiv(c)) {

						cout << "Division: " << c << " at " << SuperCell::getMCS(c) << endl;
						int newSuper = grid.cleaveCell(c);
						grid.fullTextureRefresh();

						SuperCell::setNextDiv(c, (int)RandomNumberGenerators::rNormalFloat(MCS_M_DIV_TARGET, SD_M_DIV_TARGET));
						SuperCell::setNextDiv(newSuper, (int)RandomNumberGenerators::rNormalFloat(MCS_M_DIV_TARGET, SD_M_DIV_TARGET));

					}

				}

			}

			//Compaction stage
			if (m >= compactionTime) {
				for (int c = (int)CELL_TYPE::GENERIC; c < SuperCell::getCounter(); c++) {

					if (SuperCell::getCellType(c) == CELL_TYPE::GENERIC) {
						
						SuperCell::setCellType(c, CELL_TYPE::GENERIC_COMPACT);

					}

				}
				cout << "Compaction at: " << m << endl;
				compacted = true;
			}

		}	

		if (compacted && !differentationA) {
				
			if (m >= differentiationTime) {

				cout << "Differentiation at: " << m << endl;
				
				for (int y = 1; y <= grid.interiorHeight; y++) {
					for (int x = 1; x <= grid.interiorWidth; x++) {

						Cell& c = grid.getCell(x, y);

						if (c.getType() == CELL_TYPE::GENERIC_COMPACT) {
							
							auto N = grid.getNeighbours(x,y,CELL_TYPE::EMPTYSPACE);
							
							if (!N.empty()) {
																
								c.setType(CELL_TYPE::TROPHECTODERM);
								c.generateNewColour();
								c.setNextDiv((int)RandomNumberGenerators::rNormalFloat(funcTrophectoderm(0), SD_T_DIV_TARGET));
								c.setMCS(0);

							}

						}

					}

				}

				for (int y = 1; y <= grid.interiorHeight; y++) {
					for (int x = 1; x <= grid.interiorWidth; x++) {

						Cell& c = grid.getCell(x, y);

						if (c.getType() == CELL_TYPE::GENERIC_COMPACT) {

							c.setType(CELL_TYPE::ICM);
							c.generateNewColour();
							c.setNextDiv((int)RandomNumberGenerators::rNormalFloat(MCS_I_DIV_TARGET, SD_I_DIV_TARGET));
							c.setMCS(0);
							
						}

					}

				}

				bool fluidCreation = false;
				while (!fluidCreation) {

					int x = RandomNumberGenerators::rUnifInt(1, grid.interiorWidth);
					int y = RandomNumberGenerators::rUnifInt(1, grid.interiorHeight);

					Cell& c = grid.getCell(x, y);

					if (c.getType() == CELL_TYPE::ICM) {

						c.setSuperCell((int) CELL_TYPE::FLUID);
						SuperCell::setColour((int)CELL_TYPE::FLUID, vector<int> {154, 102, 102, 255});
												
						fluidCreation = true;
						
					}

				}

				diffStartMCS = m;
				differentationA = true;
				grid.fullTextureRefresh();

			}

		}

		if (differentationA) {

			for (int c = (int)CELL_TYPE::GENERIC; c < SuperCell::getCounter(); c++) {

				if (SuperCell::getCellType(c) == CELL_TYPE::TROPHECTODERM) {

					if (SuperCell::getMCS(c) >= SuperCell::getNextDiv(c)) {

						bool divideSuccess = false;
						vector<reference_wrapper<Cell>> cList;

						for (int x = 0; x <= grid.interiorWidth; x++) {

							for (int y = 0; y <= grid.interiorHeight; y++) {

								Cell& activeCell = grid.getCell(x, y);

								if (activeCell.getSuperCell() == c) {

									auto N = grid.getNeighbours(x, y, CELL_TYPE::EMPTYSPACE);

									cList.push_back(activeCell);

									if (!N.empty()) {

										cout << "Division: " << c << " at " << SuperCell::getMCS(c) << endl;
										int newSuper = grid.divideCellRandomAxis(c);
										
										SuperCell::setNextDiv(newSuper, (int)RandomNumberGenerators::rNormalFloat(funcTrophectoderm(m - diffStartMCS), SD_T_DIV_TARGET));

										
										divideSuccess = true;
										goto endLoop;

									}

								}

							}

						}

						if (!divideSuccess) {
							for (int L = 0; L < cList.size(); L++) {
								cList[L].get().setSuperCell((int)CELL_TYPE::FLUID);
							}
						}

						endLoop:
											
						SuperCell::setNextDiv(c, (int)RandomNumberGenerators::rNormalFloat(funcTrophectoderm(m-diffStartMCS), SD_T_DIV_TARGET));
						grid.fullTextureRefresh();

					}

				} else if(SuperCell::getCellType(c) == CELL_TYPE::ICM) {

					if (SuperCell::getMCS(c) >= SuperCell::getNextDiv(c)) {

						cout << "Division: " << c << " at " << SuperCell::getMCS(c) << endl;
						int newSuper = grid.divideCellShortAxis(c);
						grid.fullTextureRefresh();

						SuperCell::setNextDiv(c, (int)RandomNumberGenerators::rNormalFloat(MCS_I_DIV_TARGET, SD_I_DIV_TARGET));
						SuperCell::setNextDiv(newSuper, (int)RandomNumberGenerators::rNormalFloat(MCS_I_DIV_TARGET, SD_I_DIV_TARGET));


					}

				}

			}

			//Fluid expansion
			int newFluidVolume = max(50,(int) (TARGET_MAX_FLUID*(1-exp(-((m-diffStartMCS)/(TARGET_SCALE_FLUID))))));
			SuperCell::setTargetVolume((int)CELL_TYPE::FLUID, newFluidVolume);
			
		}
		
		//Artificial delay if desired
		if (SIM_DELAY != 0) std::this_thread::sleep_for(std::chrono::milliseconds(SIM_DELAY));

		if (done) {
			break;
		}

		//Increase MCS count for each cell
		SuperCell::increaseMCS();

	}

	done = true;
	
	return 0;

}

int simInit(int argc, char* argv[]) {

	try {

		po::options_description description("Simulation options:");
		description.add_options()
			("help", "Display this help message")
			("maxMCS,i", po::value<unsigned int>()->default_value((int) (6*24*MCS_HOUR_EST)), "Number of MCS")
			("pixel,p", po::value<unsigned int>()->default_value(4), "Pixels per cell")
			("height,h", po::value<unsigned int>()->default_value(150), "Simulation space height")
			("width,w", po::value<unsigned int>()->default_value(150), "Simulation space width")
			("delay,d", po::value<unsigned int>()->default_value(0), "Simulation artificial delay, arbitrary, play around for good values, zero for as fast as possible")
			("fps,f", po::value<unsigned int>()->default_value(30), "Simulation target fps, default 30");

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(description).allow_unregistered().run(), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << description;
			return 1;
		}

		if (vm.count("maxMCS")) {
			MAX_MCS = vm["maxMCS"].as<unsigned int>();
		}

		if (vm.count("pixel")) {
			PIXEL_SCALE = vm["pixel"].as<unsigned int>();
		}

		if (vm.count("height")) {
			SIM_HEIGHT = vm["height"].as<unsigned int>();
		}

		if (vm.count("width")) {
			SIM_WIDTH = vm["width"].as<unsigned int>();
		}

		if (vm.count("delay")) {
			SIM_DELAY = vm["delay"].as<unsigned int>();
		}

		if (vm.count("fps")) {
			RENDER_FPS = vm["fps"].as<unsigned int>();
		}

		return 0;

	}
	catch (const std::exception& e) {
		cout << "Unexpected argument" << endl;
		cout << "use --help for help" << endl;
		cout << e.what();
		return 1;
	}

}

int printGrid(SDL_Renderer* renderer, SDL_Texture* texture, SquareCellGrid& grid) {
	
	std::vector<unsigned char> pixels = grid.getPixels();

	SDL_UpdateTexture(texture, NULL, pixels.data(), grid.boundaryWidth * 4);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

	return 0;

}