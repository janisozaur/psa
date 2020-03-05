#include "Addresses.h"
#include "MemoryStream.h"
#include "structs.h"
#include "psa_openrct2.h"

#include <benchmark/benchmark.h>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#ifdef __linux
    #include <sys/mman.h>
#elif defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif
#include <zlib.h>

#define RCT2_ADDRESS_CURRENT_ROTATION 0x0141E9E0

#ifdef __linux
static bool platform_file_exists(const utf8* path)
{
    bool exists = access(path, F_OK) != -1;
    return exists;
}
#elif defined(_WIN32)
static bool platform_file_exists(const utf8* path)
{
    DWORD result = GetFileAttributesA(path);
    DWORD error = GetLastError();
    return !(result == INVALID_FILE_ATTRIBUTES && (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND));
}
#endif

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
    uLongf cb{};
    size_t res = fread(&cb, 4, 1, file);
    if (res != 1) {
        std::cout << "Invalid read, expected 1 element, got " << res << std::endl;
        return {};
    }
    fseek(file, 0, SEEK_END);
    uLong fsize = ftell(file) - 4;
    fseek(file, 4, SEEK_SET);
    uint32_t org_cb = cb;
    auto compressedBuffer = std::make_unique<uint8_t[]>(fsize);
    res = fread(compressedBuffer.get(), 1, fsize, file);
    if (res != fsize) {
        std::cout << "Invalid read, expected " << fsize << " elements, got " << res << std::endl;
        return {};
    }
    fclose(file);
    auto buffer = std::make_unique<uint8_t[]>(cb);
    int dcpr_result = uncompress(buffer.get(), &cb, compressedBuffer.get(), fsize);
    if (cb != org_cb || Z_OK != dcpr_result) {
        std::cout << "Invalid decompressed size " << cb << ", expected " << org_cb << ", dcpr_result = " << dcpr_result << std::endl;
        auto stringifier = [dcpr_result](){
            switch (dcpr_result) {
                case Z_OK:
                    return "Z_OK";
                case Z_MEM_ERROR:
                    return "Z_MEM_ERROR";
                case Z_BUF_ERROR:
                    return "Z_BUF_ERROR";
                case Z_DATA_ERROR:
                    return "Z_DATA_ERROR";
                default:
                    return "unknown";
            };
        };
        std::cout << "dcpr_result = " << stringifier() << std::endl;
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

static std::string paint_struct_list_to_string(const paint_struct* ps, const paint_struct* base)
{
    std::string result;
    while (ps) {
        result += std::to_string(ps - base) + ";";
        ps = ps->next_quadrant_ps;
    }
    return result;
}

static bool verify(const std::vector<paint_session> inputSessions)
{
    constexpr int session_to_use = 0;
    std::vector<paint_session> sessions = inputSessions;
    // Fixing up the pointers continuously is wasteful. Fix it up once for `sessions` and store a copy.
    // Keep in mind we need bit-exact copy, as the lists use pointers.
    // Once sorted, just restore the copy with the original fixed-up version.
    paint_session* local_s = new paint_session[std::size(sessions)];
    fixup_pointers(&sessions[0], std::size(sessions), std::size(local_s->PaintStructs), std::size(local_s->Quadrants));
    std::copy_n(sessions.cbegin(), std::size(sessions), local_s);
    {
        std::copy_n(local_s, std::size(sessions), sessions.begin());
        paint_session_arrange(&sessions[session_to_use]);
    }
    auto result1 = paint_struct_list_to_string(sessions[session_to_use].PaintHead.next_quadrant_ps, &sessions[session_to_use].PaintStructs[0].basic);
    {
        std::copy_n(local_s, std::size(sessions), sessions.begin());
        paint_session_arrange_opt(&sessions[session_to_use]);
    }
    auto result2 = paint_struct_list_to_string(sessions[session_to_use].PaintHead.next_quadrant_ps, &sessions[session_to_use].PaintStructs[0].basic);

    std::string result3;
#if defined(__i386__) || defined(_M_IX86)
    {
        std::copy_n(local_s, std::size(sessions), sessions.begin());
        paint_struct ps;
        RCT2_GLOBAL(0x00EE7888, paint_struct*) = &ps;
        RCT2_GLOBAL(0x00F1AD0C, uint32_t) = sessions[session_to_use].QuadrantBackIndex;
        RCT2_GLOBAL(0x00F1AD10, uint32_t) = sessions[session_to_use].QuadrantFrontIndex;
        RCT2_GLOBAL(0x00EE7880, paint_entry *) = &sessions[session_to_use].PaintStructs[4000 - 1];
        // Not actually required, as the code only iterates over the pointees from quadrants.
        //memcpy(RCT2_ADDRESS(0x00EE788C, paint_struct), &sessions[session_to_use].PaintStructs[0].basic, 4000 * sizeof(paint_struct));
        memcpy(RCT2_ADDRESS(0x00F1A50C, paint_struct), &sessions[session_to_use].Quadrants[0], 512 * sizeof(paint_struct *));
        RCT2_GLOBAL(0x00EE7884, paint_struct*) = nullptr;
        RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_ROTATION, uint32_t) = sessions[session_to_use].CurrentRotation;
        RCT2_CALLPROC_X(0x688217, 0, 0, 0, 0, 0, 0, 0);
        result3 = paint_struct_list_to_string(ps.next_quadrant_ps, &sessions[session_to_use].PaintStructs[0].basic);
    }
#endif
    bool ok = true;
    std::cout << "r1: " << result1 << std::endl;
    std::cout << "r2: " << result2 << std::endl;
    std::cout << "r3: " << result3 << std::endl;
    if (result1 != result2) {
        std::cout << "error 1" << std::endl;
        ok = false;
    }
#if defined(__i386__) || defined(_M_IX86)
    if (result2 != result3) {
        std::cout << "error 2" << std::endl;
        ok = false;
    }
    if (result1 != result3) {
        std::cout << "error 3" << std::endl;
        ok = false;
    }
#endif

    delete[] local_s;
    return ok;
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
        for (int i = 0; i < std::size(sessions); i++) {
            // Provide fair conditions for vanilla and pause like it pauses
            state.PauseTiming();
            state.ResumeTiming();
            paint_session_arrange(&sessions[i]);
        }
        benchmark::DoNotOptimize(sessions);
    }
    state.SetItemsProcessed(state.iterations() * std::size(sessions));
    delete[] local_s;
}

static void BM_paint_session_arrange_opt(benchmark::State& state, const std::vector<paint_session> inputSessions)
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
        for (int i = 0; i < std::size(sessions); i++) {
            // Provide fair conditions for vanilla and pause like it pauses
            state.PauseTiming();
            state.ResumeTiming();
            paint_session_arrange_opt(&sessions[i]);
        }
        benchmark::DoNotOptimize(sessions);
    }
    state.SetItemsProcessed(state.iterations() * std::size(sessions));
    delete[] local_s;
}

#if defined(__i386__) || defined(_M_IX86)
// Based a lot on https://github.com/OpenRCT2/OpenRCT2/commit/d6fd03070268a21547f18bec8a0c87abcf30eef2
static void BM_paint_session_arrange_vanilla(benchmark::State& state, const std::vector<paint_session> inputSessions)
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
        for (int i = 0; i < std::size(sessions); i++) {
            state.PauseTiming();
            paint_struct ps;
            RCT2_GLOBAL(0x00EE7888, paint_struct*) = &ps;
            RCT2_GLOBAL(0x00F1AD0C, uint32_t) = sessions[i].QuadrantBackIndex;
            RCT2_GLOBAL(0x00F1AD10, uint32_t) = sessions[i].QuadrantFrontIndex;
            RCT2_GLOBAL(0x00EE7880, paint_entry *) = &sessions[i].PaintStructs[4000 - 1];
            memcpy(RCT2_ADDRESS(0x00F1A50C, paint_struct), &sessions[i].Quadrants[0], 512 * sizeof(paint_struct *));
            // Not actually required, as the code only iterates over the pointees from quadrants.
            //memcpy(RCT2_ADDRESS(0x00EE788C, paint_struct), &sessions[i].PaintStructs[0].basic, 4000 * sizeof(paint_struct));
            RCT2_GLOBAL(0x00EE7884, paint_struct*) = nullptr;
            RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_ROTATION, uint32_t) = sessions[i].CurrentRotation;
            state.ResumeTiming();
            RCT2_CALLPROC_X(0x688217, 0, 0, 0, 0, 0, 0, 0);
        }
        benchmark::DoNotOptimize(sessions);
    }
    state.SetItemsProcessed(state.iterations() * std::size(sessions));
    delete[] local_s;
}

static void fixup()
{
#ifdef __linux
    const char * segments = (char *)(0x401000);
    int err = mprotect((void *)0x401000, 0x8a4000 - 0x401000, PROT_READ | PROT_EXEC);
	if (err != 0)
	{
		perror("mprotect");
	}
	// section: rw data
	err = mprotect((void *)0x8a4000, 0x01429000 - 0x8a4000, PROT_READ | PROT_WRITE);
	if (err != 0)
	{
		perror("mprotect");
	}
#endif
}
#else
static void fixup() {}
#endif

int main(int argc, char* argv[])
{
    fixup();
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
                if (!verify(sessions))
                {
                    //return 1;
                }
                std::string name(argv[i]);
                benchmark::RegisterBenchmark(name.c_str(), BM_paint_session_arrange, sessions);
                std::string name_opt = name + "_opt";
                benchmark::RegisterBenchmark(name_opt.c_str(), BM_paint_session_arrange_opt, sessions);
#if defined(__i386__) || defined(_M_IX86)
                name += " vanilla";
                benchmark::RegisterBenchmark(name.c_str(), BM_paint_session_arrange_vanilla, sessions);
#endif
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
