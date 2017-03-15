#include "bitmap.hpp"
#include <iostream>
#define LODEPNG_NO_COMPILE_CPP
#include "lodepng.h"
#include <algorithm>
#include "hash.hpp"

using namespace std;

Bitmap::Bitmap(const string& file, const string& name, bool premultiply, bool trim)
: name(name)
{
    //Load the png file
    unsigned char* pdata;
    unsigned int pw, ph;
    if (lodepng_decode32_file(&pdata, &pw, &ph, file.data()))
    {
        cerr << "failed to load png: " << file << endl;
        exit(EXIT_FAILURE);
    }
    int w = static_cast<int>(pw);
    int h = static_cast<int>(ph);
    uint32_t* pixels = reinterpret_cast<uint32_t*>(pdata);
    
    //Premultiply all the pixels by their alpha
    if (premultiply)
    {
        int count = w * h;
        uint32_t c,a,r,g,b;
        float m;
        for (int i = 0; i < count; ++i)
        {
			//rgba
            /*c = pixels[i];
            a = c & 0xff;
            m = static_cast<float>(a) / 255.0f;
            r = static_cast<uint32_t>(((c >> 24) & 0xff) * m);
            g = static_cast<uint32_t>(((c >> 16) & 0xff) * m);
            b = static_cast<uint32_t>(((c >> 8) & 0xff) * m);
			pixels[i] = (r << 24) | (g << 16) | (b << 8) | a;*/
			c = pixels[i];
			a = c >> 24;
			m = static_cast<float>(a) / 255.0f;
			r = static_cast<uint32_t>((c & 0xff) * m);
			g = static_cast<uint32_t>(((c >> 8) & 0xff) * m);
			b = static_cast<uint32_t>(((c >> 16) & 0xff) * m);
			pixels[i] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    }
    
    //TODO: skip if all corners contain opaque pixels?
    
    //Get pixel bounds
    int minX = w - 1;
    int minY = h - 1;
    int maxX = 0;
    int maxY = 0;
    if (trim)
    {
        uint32_t p;
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                p = pixels[y * w + x];
                if ((p >> 24) > 0)
                {
                    minX = min(x, minX);
                    minY = min(y, minY);
                    maxX = max(x, maxX);
                    maxY = max(y, maxY);
                }
            }
        }
        if (maxX < minX || maxY < minY)
        {
            minX = 0;
            minY = 0;
            maxX = w - 1;
            maxY = h - 1;
            cout << "image is completely transparent: " << file << endl;
        }
    }
    else
    {
        minX = 0;
        minY = 0;
        maxX = w - 1;
        maxY = h - 1;
    }
    
    //Calculate our trimmed size
    width = (maxX - minX) + 1;
    height = (maxY - minY) + 1;
    frameW = w;
    frameH = h;
    
    if (width == w && height == h)
    {
        //If we aren't trimmed, use the loaded image data
        frameX = 0;
        frameY = 0;
        data = pixels;
    }
    else
    {
        //Create the trimmed image data
        data = reinterpret_cast<uint32_t*>(calloc(width * height, sizeof(uint32_t)));
        frameX = -minX;
        frameY = -minY;
        
        //Copy trimmed pixels over to the trimmed pixel array
        for (int y = minY; y <= maxY; ++y)
            for (int x = minX; x <= maxX; ++x)
                data[(y - minY) * width + (x - minX)] = pixels[y * w + x];
        
        //Free the untrimmed pixels
        free(pixels);
    }
    
    //Generate a hash for the bitmap
    hash = 0;
    hashData(hash, reinterpret_cast<char*>(data), sizeof(uint32_t) * width * height);
}

Bitmap::Bitmap(int width, int height)
: width(width), height(height)
{
    data = reinterpret_cast<uint32_t*>(calloc(width * height, sizeof(uint32_t)));
}

Bitmap::~Bitmap()
{
    free(data);
}

void Bitmap::SaveAs(const string& file)
{
    unsigned char* pdata = reinterpret_cast<unsigned char*>(data);
    unsigned int pw = static_cast<unsigned int>(width);
    unsigned int ph = static_cast<unsigned int>(height);
    if (lodepng_encode32_file(file.data(), pdata, pw, ph))
    {
        cout << "failed to save png: " << file << endl;
        exit(EXIT_FAILURE);
    }
}

void Bitmap::CopyPixels(const Bitmap* src, int tx, int ty)
{
    for (int y = 0; y < src->height; ++y)
        for (int x = 0; x < src->width; ++x)
            data[(ty + y) * width + (tx + x)] = src->data[y * src->width + x];
}
