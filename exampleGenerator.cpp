#include <fstream>
#include <vector>

// function to write an 8-byte integer into a file
void write8byteInteger(std::ofstream& file, uint64_t num) {
    for (int i = 0; i < 8; i++) {
        file.put((num >> (i * 8)) & 0xFF);
    }
}

// function to write an pixel (RGB) into a file
void writePixel(std::ofstream& file, uint8_t r, uint8_t g, uint8_t b) {
    file.put(r);
    file.put(g);
    file.put(b);
}

int main() {
    std::ofstream file("example.ciff", std::ios::binary);

    if (!file) {
        //std::cout << "Failed to open file\n";
        return 1;
    }

    std::string magic = "CIFF";
    uint64_t width = 200;
    uint64_t height = 200;
    uint64_t headerSize = 4 + 8 * 4 + 1 + 1; // magic + 4 * 8-byte integers + caption ('\n') + tags ('\0')
    uint64_t contentSize = width * height * 3; // width * height * 3 (RGB)

    file.write(magic.c_str(), 4); // Write magic
    write8byteInteger(file, headerSize); // Write header size
    write8byteInteger(file, contentSize); // Write content size
    write8byteInteger(file, width); // Write width
    write8byteInteger(file, height); // Write height

    // Write caption and tags
    file.put('\n');
    file.put('\0');

    // Write pixels (all white)
    for (uint64_t i = 0; i < width * height; i++) {
        writePixel(file, 255, 255, 255);
    }

    return 0;
}
