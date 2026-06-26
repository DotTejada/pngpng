#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <bit>
#include <vector>
using namespace std;

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

unsigned long update_crc(unsigned long crc, unsigned char *buf, int len) {
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
unsigned long crc(unsigned char *buf, int len) {
  return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

int main() {
    std::ifstream file("ff.png");

    if (!file.is_open()) {
        cerr << "Error opening file!" << endl;
        return 1;
    }

    char buf[1024];
    char SIG[8];
    unsigned char SIG_TEST[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (file.read(SIG, 8)) {
      if (memcmp(SIG, SIG_TEST, 8) == 0) {
        cout << "SIGNATURE FOUND" << endl;
      }
    }
    uint32_t length;
    if (file.read(reinterpret_cast<char*>(&length), 4)) {
      length = byteswap(length);
      cout << "LENGTH: " << length << endl;
    }
    vector<char> chunk_type(4);
    if (file.read(chunk_type.data(), 4)) {
      cout << "CHUNK_TYPE: " << endl;
      for (int i = 0; i < chunk_type.size(); i++) {
        printf("%02X ", (uint8_t)chunk_type[i]);
      }
      cout << endl;
    }
    vector<char> chunk_data(length);
    if (file.read(chunk_data.data(), length)) {
      cout << "DATA: " << endl;
      for (int i = 0; i < chunk_data.size(); i++) {
        printf("%02X ", (uint8_t)chunk_data[i]);
      }
      cout << endl;
    }
    size_t offset;
    uint32_t width;
    memcpy(&width, chunk_data.data(), sizeof(width));
    width = byteswap(width);
    cout << "WIDTH: " << width << endl;
    offset += sizeof(width);

    uint32_t height;
    memcpy(&height, chunk_data.data() + offset, sizeof(height));
    height = byteswap(height);
    offset += sizeof(height);
    cout << "HEIGHT: " << height << endl;

    uint8_t bit_depth;
    memcpy(&bit_depth, chunk_data.data() + offset, sizeof(bit_depth));
    cout << "BIT_DEPTH: " << (unsigned short)bit_depth << endl;
    offset += sizeof(bit_depth);

    uint8_t color_type;
    memcpy(&color_type, chunk_data.data() + offset, sizeof(color_type));
    cout << "COLOR_TYPE: " << (unsigned short)color_type << endl;
    offset += sizeof(color_type);

    uint8_t compression_method;
    memcpy(&compression_method, chunk_data.data() + offset, sizeof(compression_method));
    cout << "COMPRESSION_METHOD: " << (unsigned short)compression_method << endl;
    offset += sizeof(compression_method);

    uint8_t filter_method;
    memcpy(&filter_method, chunk_data.data() + offset, sizeof(filter_method));
    cout << "FILTER_METHOD: " << (unsigned short)filter_method << endl;
    offset += sizeof(filter_method);

    uint8_t interlace_method;
    memcpy(&interlace_method, chunk_data.data() + offset, sizeof(interlace_method));
    cout << "INTERLACE_METHOD: " << (unsigned short)interlace_method << endl;
    offset += sizeof(interlace_method);
    
    vector<char> crc_vec = chunk_type;
    crc_vec.insert(crc_vec.end(), chunk_data.begin(), chunk_data.end());
    unsigned long crc_calc = crc((unsigned char*)crc_vec.data(), crc_vec.size());
    printf("CRC_CALC: %X\n", crc_calc);

    uint32_t crc_real;
    if (file.read(reinterpret_cast<char*>(&crc_real), 4)) {
      crc_real = byteswap(crc_real);
      printf("CRC_REAL: %X\n", crc_real);
    }

    if (crc_calc == crc_real) {
      cout << "CRC VALID" << endl;
    }

    //while (file.read(buf, 1024)) {
      //for (int i = 0; i < 1024; i++) {
        //cout << hex << setw(2) << setfill('0') << uppercase << (short)buf[i] << " ";
        //}
      //cout << endl;
      //}

    // Handle remaining
    //if (file.gcount() > 0 ) {
    //for (int i = 0; i < 1024; i++) {
        //cout << hex << setw(2) << setfill('0') << uppercase << (short)buf[i] << " ";
    //  }
      //cout << endl;
    //}

    file.close(); 
    return 0;
}
