#include <iostream>
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

std::unordered_map<int, uint32_t> tileIdToPackedColor = {
    {0, 4278255615},
    {1, 4288078230}
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

int main() {
    // uint32_t color = packColor(150, 225, 150);
    // std::printf("%u\n", color);
    // 4278255615 yellow
    // 4288078230 light green
    
    size_t win_w = 512;
    size_t win_h = 512;

    // gradient (not needed, just for reference)
    std::vector<uint32_t> framebuffer(win_w*win_h, 0);
    for (size_t j = 0; j < win_h; j++) {
        for (size_t i = 0; i < win_w; i++) {
            uint8_t r = 255*j/float(win_h);
            uint8_t g = 255*i/float(win_w);
            uint8_t b = 0;
            framebuffer[j * win_w + i] = packColor(r, g, b);
        }
    }
    createPPMImage("./out/gradient.ppm", framebuffer, win_w, win_h, "P6");

    std::vector<int> map;
    size_t num_w; // number of tiles (width)
    size_t num_h;
    loadMap("./csv_map.csv", map, num_w, num_h);
    std::printf("%zu by %zu\n", num_w, num_h);

    drawMap(framebuffer, win_w, win_h, map, num_w, num_h);

    createPPMImage("./out/map.ppm", framebuffer, win_w, win_h, "P6");
}
