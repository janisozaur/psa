#include "MemoryStream.h"
#include "structs.h"

#include <cstdio>
#include <cstdint>
#include <iostream>
#include <memory>
#include <zlib.h>

static paint_struct read_paint_struct(MemoryStream &ms)
{
    paint_struct ps;
    ps.bounds.x = ms.ReadValue<uint16_t>();
    ps.bounds.y = ms.ReadValue<uint16_t>();
    ps.bounds.z = ms.ReadValue<uint16_t>();
    ps.bounds.x_end = ms.ReadValue<uint16_t>();
    ps.bounds.y_end = ms.ReadValue<uint16_t>();
    ps.bounds.z_end = ms.ReadValue<uint16_t>();
    ps.next_quadrant_ps = (paint_struct *)(uintptr_t)ms.ReadValue<uint32_t>();
    ps.quadrant_flags = ms.ReadValue<uint8_t>();
    ps.quadrant_index = ms.ReadValue<uint8_t>();
    ps.x = ms.ReadValue<uint16_t>();
    ps.y = ms.ReadValue<uint16_t>();
    ps.image_id = ms.ReadValue<uint32_t>();
    return ps;
}

static std::vector<paint_session> read_sessions(MemoryStream& ms)
{
    uint32_t sessions_count = ms.ReadValue<uint32_t>();
    std::vector<paint_session> sessions(sessions_count);
    for (uint32_t i = 0; i < sessions_count; i++) {
        auto& session = sessions[i];
        for (int j = 0; j < 4000; j++) {
            session.PaintStructs[j].basic = read_paint_struct(ms);
        }
        for (int j = 0; j < 512; j++) {
            session.Quadrants[j] = (paint_struct *)(uintptr_t)ms.ReadValue<uint32_t>();
        }
        session.PaintHead = read_paint_struct(ms);
        session.QuadrantFrontIndex = ms.ReadValue<uint32_t>();
        session.QuadrantBackIndex = ms.ReadValue<uint32_t>();
    }
    return sessions;
}

int main(int argc, char* argv[])
{
    if (argc < 1) {
        std::cerr << "Please pass the paint session dump file" << std::endl;
    }
    FILE* file = fopen(argv[1], "r");
    uLongf cb;
    fread(&cb, 4, 1, file);
    fseek(file, 0, SEEK_END);
    uint64_t fsize = ftell(file) - 4;
    fseek(file, 4, SEEK_SET);
    uint32_t org_cb = cb;
    auto compressedBuffer = std::make_unique<uint8_t[]>(fsize - 4);
    fread(compressedBuffer.get(), fsize - 4, 1, file);
    auto buffer = std::make_unique<uint8_t[]>(cb);
    uncompress(buffer.get(), &cb, compressedBuffer.get(), fsize - 4);
    if (cb != org_cb) {
        std::cout << "Invalid decompressed size " << cb << ", expected " << org_cb << std::endl;
        return 1;
    }
    MemoryStream ms(buffer.get(), cb);
    std::string version(ms.ReadString());
    if (version != "paint session v1") {
        std::cout << "Invalid version: " << version << std::endl;
        return 1;
    }
    std::string park(ms.ReadString());
    std::cout << "Park " << park << std::endl;
    std::vector<paint_session> sessions = read_sessions(ms);
    if (ms.GetPosition() != ms.GetLength()) {
        std::cout << "There are " << (ms.GetLength() - ms.GetPosition()) << " leftover bytes. Consumed "
                  << ms.GetPosition() << " out of " << ms.GetLength() << std::endl;
    }
    std::cout << "ByteSizeLong " << fsize << " bytes" << std::endl;
    std::cout << "cb " << cb << " bytes" << std::endl;
    fclose(file);
}