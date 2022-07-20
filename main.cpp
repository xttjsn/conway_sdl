#include <SDL2/SDL.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <ctype.h>
#include "cellmap.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

constexpr int kCellSize = 5;
constexpr int kWindowWidth = 1080;
constexpr int kWindowHeight = 720;
constexpr int kTickRate = 50; // millisecond

SDL_Window *window = nullptr;
SDL_Surface *surface = nullptr;

void debugDrawCell(unsigned int x, unsigned int y, RGBA color)
{
	Uint8* pixel_ptr = (Uint8*)surface->pixels + (y * kCellSize * kWindowWidth + x * kCellSize) * 4;

	for (unsigned int i = 0; i < kCellSize; i++)
	{
		for (unsigned int j = 0; j < kCellSize; j++)
		{
			*(pixel_ptr + j * 4) = color.r;
			*(pixel_ptr + j * 4 + 1) = color.g;
			*(pixel_ptr + j * 4 + 2) = color.b;
		}
		pixel_ptr += kWindowWidth * 4;
	}
}

void debugUpdate() {
    for (int i = 0; i < std::min(kWindowWidth, kWindowHeight); i++) {
        debugDrawCell(i, i, kOnColor);
    }
}

int main(int argc, char *argv[])
{
#if JSON
    if (argc < 2) {
        std::cerr << "Usage: ./game <input file>\n";
        return -1;
    }

    // Read and parse json
    std::ifstream f(argv[1]);
    json json_data = json::parse(f);
    json points = json_data["data"];
#endif

#if CIN
    std::vector<XY> xys;
    Coord x, y;
    bool neg;
    std::string s_int;
    char c;
    while (std::cin >> c) {
        if (isdigit(c)) {
            s_int = s_int.append(1, c);
        }
        else if (c == ',') {
            x = neg ? -std::atol(s_int.c_str()) : std::atol(s_int.c_str());
            s_int = "";
            neg = false;
        }
        else if (c == ')') {
            y = neg ? -std::atol(s_int.c_str()) : std::atol(s_int.c_str()) ;
            s_int = "";
            neg = false;
            xys.emplace_back(x, y);
        }
        else if (c == '-') {
            neg = true;
        }
        // Ignore other cases
    }
#endif

    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Conway's Game of Life (QuadTree)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, kWindowWidth, kWindowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_MOUSE_FOCUS);
    if (!window) {
        std::cerr << "Cannot create SDL window!\n";
        std::abort();
    }
    surface = SDL_GetWindowSurface(window);
    if (!surface) {
        std::cerr << "Cannot get SDL surface!\n";
    }

    SDL_Event ev;
    bool started = false;
    bool quit = false;
    CellMap map(surface, kWindowWidth, kWindowHeight, kCellSize, 10);

    // Put points in
#if JSON
    for (json::iterator it = points.begin(); it != points.end(); ++it) {
        XY xy((*it)[0], (*it)[1]);
        map.addCell(xy);
    }
#endif

#if CIN
    for (auto& xy : xys) {
        map.addCell(xy);
    }
#endif

    while (!quit) {
        while (SDL_PollEvent(&ev) != 0) {
            if (ev.type == SDL_QUIT)
                quit = true;

            if (ev.type == SDL_KEYDOWN) {
                // Handling keyboard event
                switch(ev.key.keysym.sym) {
                case SDLK_LEFT:
#if DEBUG
                    std::cout << "LEFT! \n";
#endif
                    map.move(XY(-2, 0));
                    break;
                case SDLK_RIGHT:
#if DEBUG
                    std::cout << "RIGHT! \n";
#endif
                    map.move(XY(2, 0));
                    break;
                case SDLK_UP:
#if DEBUG
                    std::cout << "UP! \n";
#endif
                    map.move(XY(0, -2));
                    break;
                case SDLK_DOWN:
#if DEBUG
                    std::cout << "DOWN! \n";
#endif
                    map.move(XY(0, 2));
                    break;
                case SDLK_SPACE:
#if DEBUG
                    std::cout << "Space! \n";
#endif
                    started ^= true;
                }
            }
        }

        // // DEBUG
        // debugUpdate();
        if (started) {
            map.update();
        }

        SDL_UpdateWindowSurface(window);

        SDL_Delay(kTickRate);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
