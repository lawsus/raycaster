#include <iostream>
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <SDL2/SDL.h>

// map of unique strings to block names/files maybe?
// and another map of strings for name of blocks to unique strings
// or integers, figure out integer vs string access for maps.
std::unordered_map<int, uint32_t> tileIdToPackedColor = {
    {0, 4278255615}, // empty
    {1, 4288078230} // wall
};

uint32_t packColor(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a=255) {
    return (a << 24) + (b << 16) + (g << 8) + r;
}

void unpackColor(const uint32_t &color, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) {
    r = color;
    g = color >> 8;
    b = color >> 16;
    a = color >> 24;
}

void createPPMImage(const std::string filename, const std::vector<uint32_t> &image, const size_t w, const size_t h, const std::string type = "P6") {
    assert(image.size() == w * h);
    std::ofstream ofs(filename, std::ios::binary);
    ofs << type << "\n" << w << " " << h << "\n255\n";
    std::printf("Saving %s as %s PPM.\n", filename.c_str(), type.c_str());

    if (type != "P3" && type != "P6") {
        throw std::invalid_argument("Unsupported PPM type: " + type + ". Use 'P3' or 'P6'.");
    }

    if (type == "P6") {
        for (size_t i = 0; i < image.size(); i++) {
            uint8_t r, g, b, a;
            unpackColor(image[i], r, g, b, a);
            ofs << static_cast<char>(r) << static_cast<char>(g) << static_cast<char>(b);
        }
        ofs.close();
    } else { // P3
        for (size_t i = 0; i < image.size(); i++) {
            uint8_t r, g, b, a;
            unpackColor(image[i], r, g, b, a);
            ofs << static_cast<int>(r) << " " << static_cast<int>(g) << " " <<  static_cast<int>(b) << " ";
            if ((i + 1) % w == 0) {
                ofs << "\n";
            }
        }
        ofs.close();
    }
}

// eventually save map data as binary, but keep it human readable before I make a tilemap editor
void loadMap(const std::string filename, std::vector<int> &map, size_t &num_w, size_t &num_h) {
    std::ifstream ifs(filename);
    std::string current_line;
    num_w = -1;
    num_h = 0;
    while (getline(ifs, current_line)) {
        current_line += ' ';
        std::string current_id;
        for (char c: current_line) {
            if (isdigit(c)) {
                current_id += c;
            } else if (current_id.length() > 0) {
                map.push_back(std::stoi(current_id));
                current_id = "";
            }
        }
        num_h += 1;
    }
    num_w = map.size() / num_h;
}

void drawTile(std::vector<uint32_t> &framebuffer, const size_t win_w, const size_t win_h, const size_t x, const size_t y, const size_t tile_w, const size_t tile_h, const int tile_id) {
    (void) win_h;
    for (size_t j = y; j < y + tile_h; j++) {
        for (size_t i = x; i < x + tile_w; i++) {
            framebuffer[j * win_w + i] = tileIdToPackedColor[tile_id];
        }
    }
}

void drawMap(std::vector<uint32_t> &framebuffer, const size_t win_w, const size_t win_h, std::vector<int> &map, const size_t num_w, const size_t num_h) {
    size_t tile_w = win_w / num_w;
    size_t tile_h = win_h / num_h;
    int tile_id;
    for (size_t j = 0; j < num_h; j++) {
        for (size_t i = 0; i < num_w; i++) {
            tile_id = map[j * num_w + i];
            drawTile(framebuffer, win_w, win_h, tile_w * i, tile_h * j, tile_w, tile_h, tile_id);
        }
    }
}

void drawPlayer(std::vector<uint32_t> &framebuffer, const size_t win_w, const size_t win_h, const float player_x, const float player_y) {
    (void) win_h;
    size_t playerSize = 5; // make this an input and draw a circle (check bounds -> then draw)
    for (size_t j = player_y - playerSize; j < player_y + playerSize; j++) {
        for (size_t i = player_x - playerSize; i < player_x + playerSize; i++) {
            framebuffer[j * win_w + i] = packColor(0, 0, 0);
        }
    }
}

// draw rays to map tiles or pixels in framebuffer?
// which way for collision or multiplayer?
void drawRays(std::vector<int> &map, std::vector<uint32_t> &framebuffer, const size_t win_w, const size_t win_h, const float player_x, const float player_y, const float player_a) {
    (void) map;
    (void) framebuffer;
    (void) win_h;
    float c = 0;
    for (; c < 10000; c += 0.05) {
        float x = player_x + c * cos(player_a);
        float y = player_y + c * sin(player_a);
        // printf("position in map %zu\n", int(x) + int(y) * win_w);
        if (framebuffer[int(x) + int(y) * win_w] == tileIdToPackedColor[1]) {
            break;
        }
        framebuffer[int(x) + int(y) * win_w] = packColor(0, 0, 0);
    }
}

int getTileId(const uint32_t &color) {
    for (const auto &pair : tileIdToPackedColor) {
        if (pair.second == color) {
            return pair.first;
        }
    }
    return 0;
}

std::pair<float, int> getInfo(const std::vector<uint32_t> &framebuffer, const size_t win_w, const size_t win_h, const float player_x, const float player_y, const float player_a) {
    (void) framebuffer;
    (void) win_h;
    float c = 0;
    int tileId = 0; // default to 0 if no tile is hit
    for (; c < 10000; c += 0.05) {
        float x = player_x + c * cos(player_a);
        float y = player_y + c * sin(player_a);
        // additional check because I drew the player in black
        if (framebuffer[int(x) + int(y) * win_w] != tileIdToPackedColor[0] && framebuffer[int(x) + int(y) * win_w] != packColor(0, 0, 0)) {
            tileId = getTileId(framebuffer[int(x) + int(y) * win_w]);
            break;
        }
    }
    return std::make_pair(c, tileId);
}

// make player info a struct
void updateFrame(std::vector<uint32_t> &framebuffer, const size_t win_w, const size_t win_h, const std::vector<uint32_t> &map, const float player_x, const float player_y, const float player_a) {
    std::vector<std::pair<float, int>> info_vector;
    const float fov = M_PI/3; // pi/3 = 60 degrees
    const float angle_step = fov/win_w; // angle between each ray
    for (size_t i = 0; i < win_w; i++) { // for each pixel column
        float a = player_a - fov/2 + angle_step * i; // does from player_a - fov/2 to player_a + fov/2
        std::pair<float, int> info = getInfo(map, win_w, win_h, player_x, player_y, a);
        // correct fisheye effect
        float corrected_distance = info.first * cos(player_a - a);
        info.first = corrected_distance;
        info_vector.push_back(info);
    }
    for (size_t i = 0; i < info_vector.size(); i++) {
        float distance = info_vector[i].first;
        int tileId = info_vector[i].second;
        if (distance <= 0) {
            distance = 0.0001;
        }
        float distance_scale = 0.03;
        float wall_height = win_h / (distance * distance_scale);
        size_t wall_start = (win_h - wall_height) / 2;
        size_t wall_end = wall_start + wall_height;
        if (tileId != 0) {
            for (size_t j = 0; j < win_h; j++) {
                // scratch tileIdToPackedColor, will replace with strings to textures anyways
                if (j < wall_start) { // sky
                    framebuffer[j * win_w + i] = packColor(140, 190, 240);
                } else if (wall_start <= j && j <= wall_end) { // wall
                    framebuffer[j * win_w + i] = packColor(90, 100, 110);
                } else { // ground
                    framebuffer[j * win_w + i] = packColor(35, 125, 35);
                }
            }
        }
    }
}

// int main() {
//     // uint32_t color = packColor(150, 225, 150);
//     // std::printf("%u\n", color);
//     // 4278255615 yellow
//     // 4288078230 light green
    
//     size_t win_w = 512;
//     size_t win_h = 512;

//     // gradient (not needed, just for reference)
//     std::vector<uint32_t> framebuffer(win_w*win_h, 0);
//     for (size_t j = 0; j < win_h; j++) {
//         for (size_t i = 0; i < win_w; i++) {
//             uint8_t r = 255*j/float(win_h);
//             uint8_t g = 255*i/float(win_w);
//             uint8_t b = 0;
//             framebuffer[j * win_w + i] = packColor(r, g, b);
//         }
//     }
//     createPPMImage("./out/gradient.ppm", framebuffer, win_w, win_h, "P6");

//     std::vector<int> map;
//     size_t num_w; // number of tiles (width)
//     size_t num_h;
//     loadMap("./csv_map.csv", map, num_w, num_h);
//     std::printf("Loaded map with dimensions %zu by %zu\n", num_w, num_h);

//     drawMap(framebuffer, win_w, win_h, map, num_w, num_h);

//     float player_x = win_w/2;
//     float player_y = win_h/2;
//     float player_a = 0;
//     drawPlayer(framebuffer, win_w, win_h, player_x, player_y);
//     // map/grid class pairing framebuffer and its dimensions
    
//     // Draw top down map with rays
//     std::vector<std::pair<float, int>> info;
//     // we need a ray to each pixel column (win_w)
//     const float fov = M_PI/3; // pi/3 = 60 degrees
//     const float angle_step = fov/win_w; // angle between each ray
//     for (size_t i = 0; i < win_w; i++) { // for each pixel column
//         float a = player_a - fov/2 + angle_step * i; // does from player_a - fov/2 to player_a + fov/2
//         info.push_back(getInfo(framebuffer, win_w, win_h, player_x, player_y, a));
//         drawRays(map, framebuffer, win_w, win_h, player_x, player_y, a);
//     }
//     createPPMImage("./out/map.ppm", framebuffer, win_w, win_h, "P6");
    
// }

// segementation fault when move by holding down a and s immediately after spawn

int main() {
    // initialize SDL
    SDL_Init(SDL_INIT_VIDEO);
    // create window
    SDL_Window *window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 512, SDL_WINDOW_SHOWN);
    // create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // create texture for framebuffer
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 512, 512);

    size_t win_w = 512;
    size_t win_h = 512;

    std::vector<int> map;
    size_t num_w;
    size_t num_h;
    loadMap("./csv_map.csv", map, num_w, num_h);
    std::printf("Loaded map with dimensions %zu by %zu\n", num_w, num_h);

    std::vector<uint32_t> map_framebuffer(win_w*win_h, 0);
    drawMap(map_framebuffer, win_w, win_h, map, num_w, num_h);

    float player_x = win_w/2;
    float player_y = win_h/2;
    float player_a = 0;
    float player_move_speed = 10;

    std::vector<uint32_t> framebuffer(win_w*win_h, 0);

    SDL_Event event;
    bool running = true;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEMOTION) {
                player_a += event.motion.xrel * 0.001;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                }
            }
        }

        float move_x = cos(player_a) * player_move_speed;
        float move_y = sin(player_a) * player_move_speed;

        float old_player_x = player_x;
        float old_player_y = player_y;

        if (keys[SDL_SCANCODE_W]) {
            player_x += move_x;
            player_y += move_y;
        }
        if (keys[SDL_SCANCODE_S]) {
            player_x -= move_x;
            player_y -= move_y;
        }
        if (keys[SDL_SCANCODE_A]) {
            player_x += move_y;
            player_y -= move_x;
        }
        if (keys[SDL_SCANCODE_D]) {
            player_x -= move_y;
            player_y += move_x;
        }

        // bounds check is funky right now
        if (player_x < 0 || player_x > win_w) {
            player_x = old_player_x;
        }
        if (player_y < 0 || player_y > win_h) {
            player_y = old_player_y;
        }

        // update framebuffer
        updateFrame(framebuffer, win_w, win_h, map_framebuffer, player_x, player_y, player_a);

        // update texture with updated framebuffer
        SDL_UpdateTexture(texture, NULL, framebuffer.data(), win_w * sizeof(uint32_t));
        // clear screen
        SDL_RenderClear(renderer);
        // copy texture to screen
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        // update screen
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();    

    return 0;
}
