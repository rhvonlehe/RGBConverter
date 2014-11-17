#include <iostream>
#include <fstream>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <array>

//#define NDEBUG

using namespace std;

typedef union
{
    uint32_t value;
    int8_t bytes[4];
} tIntBytesUnion;

const uint32_t bytesToRead = 480 * 640 * 2;
const uint32_t totalPixels = 480 * 640;

const array<uint8_t, 54> romulusHdr =
{
    0x42, 0x4D, 0x36, 0x10, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

struct BITMAPFILEHEADER
{
    uint16_t bfType;         //< This field keeps BMP_FILE_SIGNATURE
    uint32_t bfSize;         //< This field keeps size of the file (in bytes)
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;      //< File offset to bitmap pixels
} ;

struct BITMAPINFOHEADER
{
    uint32_t biSize;         //< size of this structure
    int32_t  biWidth;        //< bitmap width in pixels
    int32_t  biHeight;       //< bitmap height in pixels
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;    //< size of pixel data.
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

struct BITMAPINFO_RGB565
{
    struct BITMAPINFOHEADER     bmpHeader;
    uint32_t                    redMask;
    uint32_t                    greenMask;
    uint32_t                    blueMask;
};

struct BITMAP_DESCRIPTOR
{
    struct BITMAPFILEHEADER     fileHeader;     //< file header goes first
    struct BITMAPINFO_RGB565    bmpInfoRGB565;  //< RGB565 bitmap info goes next
};



void convert(ifstream& infile, ofstream& outfile)
{
    uint16_t pixel_RGB565;
    //uint32_t pixel_RGB32;
    uint8_t pixel_RGB24[3];
    uint32_t red8, green8, blue8;
    uint32_t pixelCount = 0;

    // Read past the header
    //
    infile.seekg(sizeof(BITMAP_DESCRIPTOR), ios::beg);

    // Now read 2 bytes at a time, convert, write 3 bytes
    //
    while (infile.read((char*)&pixel_RGB565, sizeof(pixel_RGB565)) && (pixelCount < bytesToRead))
    {
//        cout << "I am here" << endl;

        red8 = ((pixel_RGB565 & (0x1F << 11)) >> 8);
        green8 = ((pixel_RGB565 & (0x3F << 5)) >> 3);
        blue8 = ((pixel_RGB565 & (0x1F << 0)) << 3);

        pixel_RGB24[0] = red8;
        pixel_RGB24[1] = green8;
        pixel_RGB24[2] = blue8;

//        cout << "red, gree, blue" << endl;
//        cout << red8 << " " << green8 << " " << blue8 << endl;

        outfile.write((char*)pixel_RGB24, 3);

        //pixel_RGB32 = ((Red8 << 16) | (Green8 << 8)|( Blue8 << 0));
        pixelCount += 2;
    }

    cerr << "Error: " << strerror(errno) << endl;


}

void readBitmap(ifstream& infile, array<array<uint16_t, 480>, 640>& values)
{
    // Read past the header
    //
    infile.seekg(sizeof(BITMAP_DESCRIPTOR), ios::beg);

    for (auto row=0; row < 480; row++)
    {
        //infile.read((char*) values[row].data(), 640*2);
        for (auto col=0; col < 640; col++)
        {
            infile.read((char*) &values[row][col], 2);
        }
#ifndef NDEBUG
        cout << "Reading row: " << row << endl;
        for (auto i=0; i < 640; i++)
        {
            cout << i << ": " << values[row][i] << endl;
        }
#endif
    }

#ifndef NDEBUG
    ofstream tempOutfile("out.txt");
    for (auto row=0; row < 480; row++)
    {
        tempOutfile << dec;
        tempOutfile << "\ntemprow is: " << row << endl;
        tempOutfile << hex;
        for (auto col=0, ctr=0; col < 640; col++)
        {
            tempOutfile << values[row][col] << " ";
            if (ctr++ > 20)
            {
                tempOutfile << endl;
                ctr = 0;
            }
        }
    }
    tempOutfile.close();
#endif

}

void convertBitmap(array<array<uint16_t, 480>, 640>& inVals,
                   array<tIntBytesUnion, totalPixels>& outVals)
{
    // Take advantage of the random access nature of arrays to translate
    // 640x480 to 480x640
    //
    uint32_t outPos = 0;
    uint16_t pixel_RGB565;
    tIntBytesUnion pixel_RGB24;
    uint32_t red8, green8, blue8;


    for (auto col=0; col < 640; col++)
    {
        for (auto row=0; row < 480; row++)
        {
            pixel_RGB565 = inVals[row][col];

            red8 = ((pixel_RGB565 & (0x1F << 11)) >> 8);
            green8 = ((pixel_RGB565 & (0x3F << 5)) >> 3);
            blue8 = ((pixel_RGB565 & (0x1F << 0)) << 3);

            pixel_RGB24.bytes[0] = red8;
            pixel_RGB24.bytes[1] = green8;
            pixel_RGB24.bytes[2] = blue8;

            outVals[outPos] = pixel_RGB24;

#ifndef NDEBUG
            cout << "row: " << row << " col: " << col << " value: " << pixel_RGB565;
            cout << endl;
            cout << "outVals[" << outPos << "] is: ";

            for (auto i=0; i < 3; i++)
            {
                cout << (int) outVals[outPos].bytes[i] << " ";
            }
            cout << endl;
#endif

            outPos++;
        }
    }

#ifndef NDEBUG
    cout << "size of output array: " << outVals.size() << endl;
#endif
}

void writeBitmap(ofstream& outfile, array<tIntBytesUnion, totalPixels>& values)
{
    // Write the constant header
    //
    outfile.write((char*) &romulusHdr, sizeof(romulusHdr));

    cout << "size of output array: " << values.size() << endl;

    for (auto n : values)
    {
        cout << "writing: " << (int)n.bytes[0] << " " << (int)n.bytes[1] << " " << (int)n.bytes[2] << endl;
        outfile.write((char*) &n.bytes[0], 3);
//        outfile.write((char*) &n.bytes[1], 1);
//        outfile.write((char*) &n.bytes[2], 1);
    }
}

int main(int, char* argv[])
{
    array<array<uint16_t, 480>, 640> rgb565Values;
    array<tIntBytesUnion, totalPixels> rgb888Values;

    cout << "Opening file: " << argv[1] << " for input." << endl;

    ifstream infile(argv[1], ios::in | ios::binary);

    cout << "Opening file: out.rgb888 for output." << endl;

    ofstream outfile("out.rgb888", ios::out | ios::binary);

    cout << "Reading input file: " << bytesToRead << " bytes" << endl;
    readBitmap(infile, rgb565Values);

    cout << "Converting from RGB565 to RGB888 and rotating" << endl;
    convertBitmap(rgb565Values, rgb888Values);

#ifndef NDEBUG
    int cnt = 0;
    cout << "Checking rgb888Values" << endl;
    for (auto n : rgb888Values)
    {
        if (0 == (cnt % 480))
        {
            cout << "rgb888Values row is: " << (cnt / 480) << endl;
        }
        cnt++;

        for (auto i=0; i < 3; i++)
        {
            cout << (int) n.bytes[i] << " ";
        }
        cout << endl;
    }
#endif

    cout << "Writing to output file" << endl;
    writeBitmap(outfile, rgb888Values);

    //convert(infile, outfile);

    cout << "Closing files..." << endl;

    infile.close();
    outfile.close();

    return 0;
}

