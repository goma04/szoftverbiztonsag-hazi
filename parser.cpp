#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <jpeglib.h>
#include <cstring>
#include <cstdint>
#include <array>

using namespace std;

// Define CIFF Header Structure
struct CIFFHeader
{
    char magic[4];
    uint64_t header_size;
    uint64_t content_size;
    uint64_t width;
    uint64_t height;
    std::string caption;
    std::vector<std::string> tags;
};

struct CIFF
{
    CIFFHeader header;
    std::vector<uint8_t> pixels;
};

struct CAFFHeader
{
    char magic[4];
    uint64_t header_size;
    uint64_t numAnimations;
};

struct CAFFCredits
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint64_t creatorLength;
    std::string creator;
};

struct CAFF
{
    CAFFHeader header;
    CAFFCredits credits;
};

void write_jpeg(std::vector<uint8_t> &pixels, uint64_t width, uint64_t height, string output_filename)
{
    // Create a libjpeg compression object
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    // Open output file
    FILE *outfile = fopen(output_filename.c_str(), "wb");
    jpeg_stdio_dest(&cinfo, outfile);

    // Set parameters for compression
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    // Start compression
    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];
    while (cinfo.next_scanline < cinfo.image_height)
    {
        row_pointer[0] = &pixels[cinfo.next_scanline * width * 3];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    // Finish compression
    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
}

std::string createJpgPath(std::string ciffPath)
{
    size_t last_dot = ciffPath.find_last_of(".");
    if (last_dot != std::string::npos)
    {
        ciffPath.erase(last_dot); // Erase from the dot onwards
    }
    ciffPath.append(".jpg"); // Append the new extension
    return ciffPath;
}
CIFFHeader read_CiffHeader(std::ifstream &file)
{
    CIFFHeader header;

    // Read Magic
    file.read(header.magic, sizeof(header.magic));

    // Read header size, content size, width, and height
    file.read((char *)&header.header_size, 8);
    file.read((char *)&header.content_size, 8);
    file.read((char *)&header.width, 8);
    file.read((char *)&header.height, 8);

    // Read Caption
    std::getline(file, header.caption, '\n');

    // Read Tags
    std::string tag;
    size_t captionSize = header.caption.size();
    int i = 0;
    int bytesWithoutTags = header.header_size - 37 - captionSize;
    while (i < bytesWithoutTags)
    {
        std::getline(file, tag, '\0');
        header.tags.push_back(tag);
        i += tag.size() + 1;
    }

    return header;
}

CAFFHeader read_CaffHeader(std::ifstream &file)
{

    /*if(!file) {
        std::cerr << "Could not open file " << argv[1] << "\n";
        return 1;
    }*/

    CAFFHeader header;

    file.read(header.magic, 4);
    file.read((char *)&header.header_size, 8);
    file.read((char *)&header.numAnimations, 8);

    return header;
}

CAFFCredits read_credits(std::ifstream &file)
{
    CAFFCredits credits;

    file.read((char *)&credits.year, 2);
    file.read((char *)&credits.month, 1);
    file.read((char *)&credits.day, 1);
    file.read((char *)&credits.hour, 1);
    file.read((char *)&credits.minute, 1);
    file.read((char *)&credits.creatorLength, 8);

    credits.creator.resize(credits.creatorLength);
    file.read((char *)&credits.creator[0], credits.creatorLength);

    return credits;
}

CIFF constructCiff(std::ifstream &file)
{
    CIFF ciff;

    // Read header
    CIFFHeader header = read_CiffHeader(file);

    // Read Pixels
    std::vector<uint8_t> pixels(header.content_size);
    file.read((char *)pixels.data(), header.content_size);

    ciff.header = header;
    ciff.pixels = pixels;

    return ciff;
}

int createJpgFromCiff(CIFF ciff, string filePath)
{
    if ((ciff.header.height == 0 || ciff.header.width == 0) && ciff.pixels.size() != 0)
        throw std::runtime_error("Wrong dimensions");

    string ciffPath = createJpgPath(filePath);
    // Convert to JPEG
    write_jpeg(ciff.pixels, ciff.header.width, ciff.header.height, ciffPath);

    return 0;
}

int handleCaff(string filePath)
{

    std::ifstream file(filePath);
    if (!file)
    {
        std::cerr << "Failed to open the file" << std::endl;
        return -1;
    }

    uint8_t id;
    uint64_t length;
    uint64_t duration;

    file.read((char *)&id, 1);
    file.read((char *)&length, 8);

    if(id < 1 || id > 3){
         throw std::runtime_error("Wrong id");
    }

    CAFFHeader header = read_CaffHeader(file);

    file.read((char *)&id, 1);
    file.read((char *)&length, 8);

    CAFFCredits credits = read_credits(file);

    for (uint i = 0; i < header.numAnimations; i++)
    {
        file.read((char *)&id, 1);
        file.read((char *)&length, 8);
        file.read((char *)&duration, 8);

        CIFF ciff = constructCiff(file);

        if (i == 0)
            createJpgFromCiff(ciff, filePath);
    }

    file.close();

    return 0;
}

int handleCiff(string filePath)
{
    std::ifstream file(filePath);
    if (!file)
    {
        std::cerr << "Failed to open the file" << std::endl;
        return -1;
    }

    // std::ifstream file(argv[1], std::ios::binary);

    // Read header
    CIFFHeader header = read_CiffHeader(file);

    // Read Pixels
    std::vector<uint8_t> pixels(header.content_size);
    file.read((char *)pixels.data(), header.content_size);

    if ((header.height == 0 || header.width == 0) && pixels.size() != 0)
        return -1;

    string ciffPath = createJpgPath(filePath);
    // Convert to JPEG
    write_jpeg(pixels, header.width, header.height, ciffPath);

    // std::cout << std::endl <<"vÉGE";
    return 0;
}

int main(int argc, char *argv[])
{
    string filePath = argv[2];
    string ciffOrCaff = argv[1];
    // std::cout << " " << argv[0] << " " << argv[1] << " " << argv[2];
    // std::cout<<"hello";
    /*if (argc != 3) {
        std::cout << "UDFASSSSSSSSSSSSSSSSSSSSSsage: ciff_to_jpg <input.ciff> <output.jpg>\n";
        std::cout << argc<<std::endl<<std::endl;
        std::cout << "Usage: ciff_to_jpg <input.ciff> <output.jpFSDDDDDDDDDg>\n";
        return -1;
    }*/

    try
    {
        if (ciffOrCaff == "-caff")
        {
            return handleCaff(filePath);
        }
        else
        {
            return handleCiff(filePath);
        }
    }
    catch (const exception &ex)
    {
        return -1;
    }

    return 0;
}
