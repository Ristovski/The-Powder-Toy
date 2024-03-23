#include "Config.h"

#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>

#include "client/GameSave.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/Snapshot.h"

#include "tracy/Tracy.hpp"

#include "xxhash.h"

#include "common/tpt-rand.h"

void readFile(ByteString filename, std::vector<char> & storage)
{
    ZoneScoped;
    std::ifstream fileStream;
    fileStream.open(filename.c_str(), std::ios::binary);
    if(fileStream.is_open())
    {
        fileStream.seekg(0, std::ios::end);
        size_t fileSize = fileStream.tellg();
        fileStream.seekg(0);

        unsigned char * tempData = new unsigned char[fileSize];
        fileStream.read((char *)tempData, fileSize);
        fileStream.close();

        std::vector<unsigned char> fileData;
        storage.clear();
        storage.insert(storage.end(), tempData, tempData+fileSize);
        delete[] tempData;
    }
}

void writeFile(ByteString filename, std::vector<char> & fileData)
{
    ZoneScoped;
    std::ofstream fileStream;
    fileStream.open(filename.c_str(), std::ios::binary);
    if(fileStream.is_open())
    {
        fileStream.write(&fileData[0], fileData.size());
        fileStream.close();
    }
}

XXH128_hash_t HashSnapshot(const Snapshot& snapshot)
{
    ZoneScoped;
    XXH3_state_t* const state = XXH3_createState();

    if (XXH3_128bits_reset(state) == XXH_ERROR) throw "Failed to reset hash state";

    XXH3_128bits_update(state, snapshot.AirPressure.data(), snapshot.AirPressure.size());
    XXH3_128bits_update(state, snapshot.AirVelocityX.data(), snapshot.AirVelocityX.size());
    XXH3_128bits_update(state, snapshot.AirVelocityY.data(), snapshot.AirVelocityY.size());
    XXH3_128bits_update(state, snapshot.AmbientHeat.data(), snapshot.AmbientHeat.size());

    XXH3_128bits_update(state, &snapshot.Particles[0], snapshot.Particles.size() * sizeof(Particle));

    XXH3_128bits_update(state, snapshot.GravVelocityX.data(), snapshot.GravVelocityX.size());
    XXH3_128bits_update(state, snapshot.GravVelocityY.data(), snapshot.GravVelocityY.size());
    XXH3_128bits_update(state, snapshot.GravValue.data(), snapshot.GravValue.size());
    XXH3_128bits_update(state, snapshot.GravMap.data(), snapshot.GravMap.size());

    XXH3_128bits_update(state, snapshot.BlockMap.data(), snapshot.BlockMap.size());
    XXH3_128bits_update(state, snapshot.ElecMap.data(), snapshot.ElecMap.size());

    XXH3_128bits_update(state, snapshot.FanVelocityX.data(), snapshot.FanVelocityX.size());
    XXH3_128bits_update(state, snapshot.FanVelocityY.data(), snapshot.FanVelocityY.size());

    XXH3_128bits_update(state, &snapshot.PortalParticles[0], snapshot.PortalParticles.size() * sizeof(Particle));

    XXH3_128bits_update(state, snapshot.WirelessData.data(), snapshot.WirelessData.size());

    XXH3_128bits_update(state, &snapshot.stickmen[0], snapshot.stickmen.size() * sizeof(playerst));

    // NOTE: Sign data is skipped

    XXH128_hash_t const hash = XXH3_128bits_digest(state);
    if (XXH3_freeState(state) == XXH_ERROR) throw "Failed to free hash state";

    return hash;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <inputFilename> <num_frames>" << std::endl;
        return 1;
    }

    // Seed RNG with static seed
    RNG().seed(0x1337);

    std::vector<char> inputFile;
    ByteString inputFilename = argv[1];
    int frames_to_sim = atoi(argv[2]);

    // Read save into buffer
    readFile(inputFilename, inputFile);

    SimulationData sd;
    GameSave* gameSave = nullptr;

    try
    {
        gameSave = new GameSave(inputFile);
    }
    catch (ParseException &e)
    {
        throw e;
    }

    if (gameSave)
    {
        Simulation* sim = new Simulation();
        sim->Load(gameSave, true, {0, 0});

        for (int i = 0; i < frames_to_sim; ++i)
        {
            sim->BeforeSim();
            sim->UpdateParticles(0, NPART);
            sim->AfterSim();
            std::cout << "Frame " << i+1 << " of " << frames_to_sim << "\t\r" << std::flush;
        }

        std::cout << std::endl;

        if(getenv("DUMP_HASH"))
        {
            auto hash = HashSnapshot(*sim->CreateSnapshot());
            std::cout << "Hash: " << std::hex << hash.high64 << hash.low64 << std::dec << std::endl;
        }
    }
    else
    {
        std::cout << "Save file invalid!" << std::endl;
    }
}
