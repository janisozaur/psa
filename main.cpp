#include <cstdio>
#include <cstdint>
#include <iostream>
#include <memory>
#include <zlib.h>

int main(int argc, char* argv[])
{
    if (argc < 1) {
        std::cerr << "Please pass the paint session dump file" << std::endl;
    }
    FILE* file = fopen(argv[1], "r");
    uLongf cb;
    fread(&cb, 4, 1, file);
    uint64_t fsize = ftell(file) - 4;
    auto compressedBuffer = std::make_unique<uint8_t[]>(fsize - 4);
    fread(compressedBuffer.get(), fsize - 4, 1, file);
    auto buffer = std::make_unique<uint8_t[]>(cb);
    uncompress(buffer.get(), &cb, compressedBuffer.get(), fsize - 4);
    std::cout << "ByteSizeLong " << fsize << " bytes" << std::endl;
    std::cout << "cb " << cb << " bytes" << std::endl;
    fclose(file);
}