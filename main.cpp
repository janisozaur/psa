#include "MemoryStream.h"
#include "structs.h"
#include "pas_openrct2.h"

#include <benchmark/benchmark.h>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <zlib.h>

static bool platform_file_exists(const utf8* path)
{
    bool exists = access(path, F_OK) != -1;
    return exists;
}

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

static void fixup_pointers(paint_session* s, size_t paint_session_entries, size_t paint_struct_entries, size_t quadrant_entries)
{
    for (size_t i = 0; i < paint_session_entries; i++)
    {
        for (size_t j = 0; j < paint_struct_entries; j++)
        {
            if (s[i].PaintStructs[j].basic.next_quadrant_ps == (paint_struct*)paint_struct_entries)
            {
                s[i].PaintStructs[j].basic.next_quadrant_ps = nullptr;
            }
            else
            {
                s[i].PaintStructs[j].basic.next_quadrant_ps = &s[i].PaintStructs
                                                                   [(uintptr_t)s[i].PaintStructs[j].basic.next_quadrant_ps]
                                                                       .basic;
            }
        }
        for (size_t j = 0; j < quadrant_entries; j++)
        {
            if (s[i].Quadrants[j] == (paint_struct*)quadrant_entries)
            {
                s[i].Quadrants[j] = nullptr;
            }
            else
            {
                s[i].Quadrants[j] = &s[i].PaintStructs[(size_t)s[i].Quadrants[j]].basic;
            }
        }
    }
}

static std::vector<paint_session> extract_paint_session(const char* fname)
{
    FILE* file = fopen(fname, "r");
    uLongf cb;
    fread(&cb, 4, 1, file);
    fseek(file, 0, SEEK_END);
    uint64_t fsize = ftell(file) - 4;
    fseek(file, 4, SEEK_SET);
    uint32_t org_cb = cb;
    auto compressedBuffer = std::make_unique<uint8_t[]>(fsize - 4);
    fread(compressedBuffer.get(), fsize - 4, 1, file);
    fclose(file);
    auto buffer = std::make_unique<uint8_t[]>(cb);
    uncompress(buffer.get(), &cb, compressedBuffer.get(), fsize - 4);
    if (cb != org_cb) {
        std::cout << "Invalid decompressed size " << cb << ", expected " << org_cb << std::endl;
        return {};
    }
    MemoryStream ms(buffer.get(), cb);
    std::string version(ms.ReadString());
    if (version != "paint session v1") {
        std::cout << "Invalid version: " << version << std::endl;
        return {};
    }
    std::string park(ms.ReadString());
    std::vector<paint_session> sessions = read_sessions(ms);
    if (ms.GetPosition() != ms.GetLength()) {
        std::cout << "WARNING: There are " << (ms.GetLength() - ms.GetPosition()) << " leftover bytes. Consumed "
                  << ms.GetPosition() << " out of " << ms.GetLength() << std::endl;
    }
    std::cout << "Park " << park << " has " << sessions.size() << " sessions" << std::endl;
    return sessions;
}

static void BM_paint_session_arrange(benchmark::State& state, const std::vector<paint_session> inputSessions)
{
    std::vector<paint_session> sessions = inputSessions;
    // Fixing up the pointers continuously is wasteful. Fix it up once for `sessions` and store a copy.
    // Keep in mind we need bit-exact copy, as the lists use pointers.
    // Once sorted, just restore the copy with the original fixed-up version.
    paint_session* local_s = new paint_session[std::size(sessions)];
    fixup_pointers(&sessions[0], std::size(sessions), std::size(local_s->PaintStructs), std::size(local_s->Quadrants));
    std::copy_n(sessions.cbegin(), std::size(sessions), local_s);
    for (auto _ : state)
    {
        state.PauseTiming();
        std::copy_n(local_s, std::size(sessions), sessions.begin());
        state.ResumeTiming();
        paint_session_arrange(&sessions[0]);
        benchmark::DoNotOptimize(sessions);
    }
    state.SetItemsProcessed(state.iterations() * std::size(sessions));
    delete[] local_s;
}

int main(int argc, char* argv[])
{
    {
        // Register some basic "baseline" benchmark
        std::vector<paint_session> sessions(1);
        for (auto& ps : sessions[0].PaintStructs)
        {
            ps.basic.next_quadrant_ps = (paint_struct*)(std::size(sessions[0].PaintStructs));
        }
        for (auto& quad : sessions[0].Quadrants)
        {
            quad = (paint_struct*)(std::size(sessions[0].Quadrants));
        }
        benchmark::RegisterBenchmark("baseline", BM_paint_session_arrange, sessions);
    }

    std::vector<char*> argv_for_benchmark;

    argv_for_benchmark.push_back(argv[0]);

    // Extract file names from argument list. If there is no such file, consider it benchmark option.
    for (int i = 1; i < argc; i++)
    {
        if (platform_file_exists(argv[i]))
        {
            // Register benchmark for sv6 if valid
            std::vector<paint_session> sessions = extract_paint_session(argv[i]);
            if (!sessions.empty())
            {
                benchmark::RegisterBenchmark(argv[i], BM_paint_session_arrange, sessions);
            }
        }
        else
        {
            argv_for_benchmark.push_back((char*)argv[i]);
        }
    }

    // Update argc with all the changes made
    argc = (int)argv_for_benchmark.size();
    ::benchmark::Initialize(&argc, &argv_for_benchmark[0]);
    if (::benchmark::ReportUnrecognizedArguments(argc, &argv_for_benchmark[0]))
        return -1;
    ::benchmark::RunSpecifiedBenchmarks();
    return 0;
}
