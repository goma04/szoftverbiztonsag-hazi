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
    std::vector<CIFF> ciffs;
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

//Creates the .jpg file name from the .ciff/.caff version
std::string createJpgName(std::string ciffPath)
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

//Creates a ciff object
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

//Creates a jpg file from a ciff object and saves it
int createJpgFromCiff(CIFF ciff, string filePath)
{
    if ((ciff.header.height == 0 || ciff.header.width == 0) && ciff.pixels.size() != 0)
        throw std::runtime_error("Wrong dimensions");

    string ciffPath = createJpgName(filePath);
    // Convert to JPEG
    write_jpeg(ciff.pixels, ciff.header.width, ciff.header.height, ciffPath);

    return 0;
}

//Finds the next appearance of the given magic string, then sets the read position to the start of this magic string
void findMagic(std::ifstream &file, const std::string magic)
{
    char c;
    std::string str;
    while (file.get(c))
    {
        str += c;
        if (str.size() > magic.size())
        {
            str.erase(str.begin());
        }
        if (str == magic)
        {
            file.seekg(-magic.size(), std::ios::cur);
            break;
        }
    }
}

//Handles the ciff scenario
CAFF handleCaff(string filePath)
{

    std::ifstream file(filePath);
    if (!file)
    {
        std::cerr << "Failed to open the file" << std::endl;
        throw runtime_error("Failed to open the file");
    }

    findMagic(file, "CAFF");
    CAFFHeader header = read_CaffHeader(file);

    CAFF caff;
    caff.header = header;
    uint8_t ciffCount = 0;

    //Read blocks
    while (!file.eof())
    {
        uint8_t id;
        uint64_t length;
        uint64_t duration;
        CAFFCredits credits;
        CIFF ciff;

        file.read((char *)&id, 1);
        file.read((char *)&length, 8);

        switch (id)
        {
        case 2:
        //Credit block
            caff.credits = read_credits(file);
            break;
        case 3:
        //CIFF block
            file.read((char *)&duration, 8);

            ciff = constructCiff(file);
            caff.ciffs.push_back(ciff);
            ciffCount++;
            break;
        default:
            throw runtime_error("Invalid block ID");
            break;
        }
        if (ciffCount == caff.header.numAnimations)
            break;
    }

    file.close();

    return caff;
}

//Cheks if the remaining number of bytes equal to the given content size
bool checkCorrectByteNumber(std::ifstream &file, int contentSize)
{
    // Get the current position
    file.seekg(0, std::ios::cur);
    std::streampos currentPos = file.tellg();

    // Go to the end of the file
    file.seekg(0, std::ios::end);
    std::streampos endPos = file.tellg();

    // Go back to the original position
    file.seekg(currentPos);

    std::streamoff remainingBytes = endPos - currentPos;

    if (remainingBytes == contentSize)
        return true;

    return false;
}

CIFF handleCiff(string filePath)
{
    CIFF ciff;
    std::ifstream file(filePath);
    if (!file)
    {
        std::cerr << "Failed to open the file" << std::endl;
        throw runtime_error("Failed to open the file");
    }

    findMagic(file, "CIFF");

    // Read header
    ciff.header = read_CiffHeader(file);

    if (!checkCorrectByteNumber(file, ciff.header.content_size))
        throw std::runtime_error("Wrong content-size");

    // Read Pixels
    std::vector<uint8_t> pixels(ciff.header.content_size);
    file.read((char *)pixels.data(), ciff.header.content_size);

     ciff.pixels= pixels;

    file.close();

    return ciff;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        throw runtime_error("Invalid argument count");
    }

    string ciffOrCaff = argv[1];
    string filePath = argv[2];

    try
    {
        if (strcmp(argv[1], "-caff") == 0)
        {
            CAFF caff = handleCaff(filePath);
            createJpgFromCiff(caff.ciffs[0], filePath);
            return 0;
        }
        else if (strcmp(argv[1], "-ciff") == 0)
        {
            CIFF ciff = handleCiff(filePath);
            createJpgFromCiff(ciff, filePath);
            return 0;
        }
        else
        {
            throw runtime_error("The program can only handle CIFF or CAFF files");
        }
    }
    catch (const exception &ex)
    {
        return -1;
    }

    return 0;
}
