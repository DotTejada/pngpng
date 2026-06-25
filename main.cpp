#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <bit>
#include <vector>
using namespace std;

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
    char chunk_type[4];
    if (file.read(chunk_type, 4)) {
      cout << "CHUNK_TYPE: " << chunk_type << endl;
    }
    vector<char> chunk_data(length);
    if (file.read(chunk_data.data(), length)) {
      cout << "DATA: " << endl;
      for (int i = 0; i < chunk_data.size(); i++) {
        printf("%02X ", (uint8_t)chunk_data[i]);
      }
      cout << endl;
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
