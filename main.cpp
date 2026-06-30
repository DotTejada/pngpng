#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <cmath>
#include <zlib.h>
#include <SDL2/SDL.h>
using namespace std;

typedef uint8_t u8;
typedef uint32_t u32;

u32 byteswap(u32 value) {
    return ((value & 0x000000FF) << 24) |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0xFF000000) >> 24);
}

/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
void make_crc_table(void) {
    unsigned long c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (unsigned long) n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below). */

unsigned long update_crc(unsigned long crc, u8 *buf, int len) {
    unsigned long c = crc;
    int n;

    if (!crc_table_computed)
        make_crc_table();
    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long crc(u8 *buf, int len) {
    return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

u8 paeth_predictor(u8 a, u8 b, u8 c) {
    int p, pa, pb, pc;
    p = (int)a + (int)b - (int)c;
    pa = abs(p - (int)a);
    pb = abs(p - (int)b);
    pc = abs(p - (int)c);
    if (pa <= pb && pa <= pc) {
        return (u8)a;
    } else if (pb <= pc) {
        return (u8)b;
    } else {
        return (u8)c;
    }
}

u8 filt(u8 filter_type, u8 x, u8 a, u8 b, u8 c) {
    switch (filter_type) {
    case 0:
        return x;
    case 1:
        return x - a;
    case 2:
        return x - b;
    case 3:
        return x - floor((a + b) / 2);
    case 4:
        return x - paeth_predictor(a, b, c);
    default:
        return x;
    }
}

u8 recon(u8 filter_type, u8 x, u8 a, u8 b, u8 c) {
    switch (filter_type) {
    case 0:
        return x;
    case 1:
        return x + a;
    case 2:
        return x + b;
    case 3:
        return x + floor((a + b) / 2);
    case 4:
        return x + paeth_predictor(a, b, c);
    default:
        return x;
    }
}

int main() {
    ifstream file("ff.png");

    if (!file.is_open()) {
        cerr << "Error opening file!" << endl;
        return 1;
    }

    char buf[1024];
    char sig[8];
    u8 sig_test[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (file.read(sig, 8)) {
        if (memcmp(sig, sig_test, 8) == 0) {
            cout << "SIGNATURE FOUND" << endl;
        } else {
            cerr << "SIGNATURE NOT FOUND" << endl;
            return 1;
        }
    }
    u32 length;
    if (file.read(reinterpret_cast<char*>(&length), 4)) {
        length = byteswap(length);
        cout << "LENGTH_IHDR: " << length << endl;
    } else {
        cerr << "FAILED TO READ LENGTH" << endl;
        return 1;
    }
    vector<char> chunk_type(4);
    if (file.read(chunk_type.data(), 4)) {
        cout << "CHUNK_TYPE: " << endl;
        for (int i = 0; i < chunk_type.size(); i++) {
            printf("%02X ", (u8)chunk_type[i]);
        }
        cout << endl;
        printf("%s", chunk_type.data());
        cout << endl;
    } else {
        cerr << "FAILED TO READ CHUNK_TYPE" << endl;
        return 1;
    }
    vector<char> chunk_data;
    chunk_data.resize(length);
    if (file.read(chunk_data.data(), length)) {
        cout << "DATA: " << endl;
        for (int i = 0; i < chunk_data.size(); i++) {
            printf("%02X ", (u8)chunk_data[i]);
        }
        cout << endl;
    } else {
        cerr << "FAILED TO READ CHUNK_DATA" << endl;
        return 1;
    }
    size_t offset;
    u32 width;
    memcpy(&width, chunk_data.data(), sizeof(width));
    width = byteswap(width);
    cout << "WIDTH: " << width << endl;
    offset += sizeof(width);

    u32 height;
    memcpy(&height, chunk_data.data() + offset, sizeof(height));
    height = byteswap(height);
    offset += sizeof(height);
    cout << "HEIGHT: " << height << endl;

    u8 bit_depth;
    memcpy(&bit_depth, chunk_data.data() + offset, sizeof(bit_depth));
    cout << "BIT_DEPTH: " << (unsigned short)bit_depth << endl;
    offset += sizeof(bit_depth);

    u8 color_type;
    memcpy(&color_type, chunk_data.data() + offset, sizeof(color_type));
    cout << "COLOR_TYPE: " << (unsigned short)color_type << endl;
    offset += sizeof(color_type);

    u8 compression_method;
    memcpy(&compression_method, chunk_data.data() + offset, sizeof(compression_method));
    cout << "COMPRESSION_METHOD: " << (unsigned short)compression_method << endl;
    if (compression_method != 0) {
        cerr << "COMPRESSION METHOD NOT SUPPORTED" << endl;
        return 1;
    }
    offset += sizeof(compression_method);

    u8 filter_method;
    memcpy(&filter_method, chunk_data.data() + offset, sizeof(filter_method));
    cout << "FILTER_METHOD: " << (unsigned short)filter_method << endl;
    if (filter_method != 0) {
        cerr << "FILTER METHOD NOT SUPPORTED" << endl;
        return 1;
    }
    offset += sizeof(filter_method);

    u8 interlace_method;
    memcpy(&interlace_method, chunk_data.data() + offset, sizeof(interlace_method));
    cout << "INTERLACE_METHOD: " << (unsigned short)interlace_method << endl;
    if (interlace_method != 0) {
        cerr << "INTERLACE METHOD NOT SUPPORTED" << endl;
        return 1;
    }
    offset += sizeof(interlace_method);
    
    vector<char> crc_vec = chunk_type;
    crc_vec.insert(crc_vec.end(), chunk_data.begin(), chunk_data.end());
    unsigned long crc_calc = crc((u8*)crc_vec.data(), crc_vec.size());
    printf("CRC_CALC: %lX\n", crc_calc);

    u32 crc_real;
    if (file.read(reinterpret_cast<char*>(&crc_real), 4)) {
        crc_real = byteswap(crc_real);
        printf("CRC_REAL: %X\n", crc_real);
    } else {
        cerr << "FAILED TO READ CRC" << endl;
        return 1;
    }

    if (crc_calc == crc_real) {
        cout << "CRC VALID" << endl;
    } else {
        cerr << "CRC INVALID" << endl;
        return 1;
    }

    vector<char> zlib_input;
    while(true) {
        cout << "==================" << endl;
        if (file.read(reinterpret_cast<char*>(&length), 4)) {
            length = byteswap(length);
            cout << "LENGTH: " << length << endl;
        } else {
            cerr << "FAILED TO READ LENGTH" << endl;
            return 1;
        }

        string ct;
        if (file.read(chunk_type.data(), 4)) {
            ct = chunk_type.data();
            cout << "CHUNK_TYPE: " << ct << endl;
        } else {
            cerr << "FAILED TO READ CHUNK_TYPE" << endl;
            return 1;
        }

        chunk_data.resize(length);
        if (file.read(chunk_data.data(), length)) {
            cout << "DATA READ SUCCESSFULLY" << endl;
        } else {
            cerr << "FAILED TO READ DATA" << endl;
            return 1;
        }

        crc_vec = chunk_type;
        crc_vec.insert(crc_vec.end(), chunk_data.begin(), chunk_data.end());
        crc_calc = crc((u8*)crc_vec.data(), crc_vec.size());

        if (file.read(reinterpret_cast<char*>(&crc_real), 4)) {
            crc_real = byteswap(crc_real);
            printf("CRC: %X\n", crc_real);
        } else {
            cerr << "FAILED TO READ CRC" << endl;
            return 1;
        }

        if (crc_calc == crc_real) {
            cout << "CRC VALID" << endl;
        } else {
            cerr << "CRC INVALID" << endl;
            return 1;
        }

        if (ct == "IDAT") {
            zlib_input.insert(zlib_input.end(), chunk_data.begin(), chunk_data.end());
        } else if (ct == "IEND") {
            break;
        } else {
            continue;
        }
    }

    vector<u8> zlib_output(height * ((width * 3) + 1));
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = zlib_input.size();
    infstream.next_in = (Bytef *)zlib_input.data();
    infstream.avail_out = zlib_output.size();
    infstream.next_out = (Bytef *)zlib_output.data();

    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    u8 filter_type, x, a, b, c;
    for (int i = 0; i < height; i++) {
        int h = ((width * 3) + 1);
        filter_type = (u8)zlib_output[i * h];
        for (int j = 1; j < (width * 3) + 1; j++) {
            x = (u8)zlib_output[j + (i * h)];
            a = (j - 3) >= 1 ? (u8)zlib_output[(j - 3) + (i * h)] : 0;
            b = (i - 1) >= 0 ? (u8)zlib_output[j + ((i - 1) * h)] : 0;
            c = ((j - 3) >= 1 && (i - 1) >= 0) ? (u8)zlib_output[(j - 3) + ((i - 1) * h)] : 0;
            zlib_output[j + (i * h)] = recon(filter_type, x, a, b, c);
        }
    }
    cout << "DECODING SUCCESSFUL" << endl;

    file.close(); 

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL ERROR" << endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("PNG VIEWER", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        cerr << "SDL ERROR" << endl;
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
        cerr << "Renderer could not be created!" << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawColor(renderer, 12, 12, 12, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < height; i++) {
        for (int j = 1; j < (width * 3) + 1; j += 3) {
            int offset = (i * ((width * 3) + 1)) + j;
            u8 r = (u8)zlib_output[offset];
            u8 g = (u8)zlib_output[offset + 1];
            u8 b = (u8)zlib_output[offset + 2];
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, (j - 1) / 3, i);
        }
    }

    SDL_RenderPresent(renderer);
    bool run = true;
    SDL_Event event;
    while (run) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                run = false;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
