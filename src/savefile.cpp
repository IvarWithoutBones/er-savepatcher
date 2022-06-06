#include "savefile.h"
#include "util.h"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fstream>
#include <span>

namespace savepatcher {

void SaveFile::validateData(std::span<u8> data, const std::string &target) {
    if (HeaderBNDSection.charsFrom(data) != "BND" || data.size_bytes() != SaveFileSize)
        throw exception("{} is not a valid Elden Ring save file.", target);
}

std::vector<u8> SaveFile::loadFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    std::vector<u8> data;
    if (!file.is_open())
        throw exception("Could not open file '{}'", filename);

    data = {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    file.close();
    return data;
}

void SaveFile::write(const std::string &filename) {
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    validateData(patchedSaveData, "Generated data");
    if (!file.is_open())
        throw exception("Could not open file '{}'", filename);

    file.write(reinterpret_cast<const char *>(patchedSaveData.data()), patchedSaveData.size());
    file.close();
}

size_t SaveFile::getActiveSlotIndex(const std::span<u8> data) const {
    auto section{ActiveSection.bytesFrom(data)};
    auto found{std::find(section.begin(), section.end(), sizeof(true))};
    if (found == std::end(section))
        throw exception("Could not find active slot index");

    return std::distance(section.begin(), found);
};

std::string SaveFile::recalculateChecksum() {
    auto saveHeaderChecksum{GenerateMd5(SaveHeaderSection.bytesFrom(patchedSaveData))};
    auto saveHeaderChecksumString{FormatHex(saveHeaderChecksum)};
    if (checksum() == saveHeaderChecksumString)
        throw exception("Save header checksum is already correct");

    SaveHeaderChecksumSection.replace(patchedSaveData, saveHeaderChecksum);

    // Used for logging
    return saveHeaderChecksumString;
}

void SaveFile::replaceSteamId(u64 inputSteamId) {
    std::array<u8, SteamIdSection.size> newSteamIdBytes;
    if (steamId() == inputSteamId)
        throw exception("Steam ID is already correct");

    // Convert the given ID to raw bytes
    // TODO: do this in a better way, I dont even understand what's going on here
    for (size_t i{}; i < SteamIdSection.size; i++)
        newSteamIdBytes[i] = inputSteamId >> (i * 8);

    SteamIdSection.replace(patchedSaveData, newSteamIdBytes);
}

std::string SaveFile::checksum(const std::span<u8> data) const {
    return FormatHex(SaveHeaderChecksumSection.bytesFrom(data));
}

std::string SaveFile::name(const std::span<u8> data) const {
    return NameSection.charsFrom(data);
}

std::string SaveFile::timePlayed(const std::span<u8> data) const {
    return SecondsToTimeStamp(SecondsPlayedSection.castInteger<u32>(data));
};

size_t SaveFile::activeSlot(const std::span<u8> data) const {
    return activeSlotIndex;
}

u64 SaveFile::steamId(const std::span<u8> data) const {
    return SteamIdSection.castInteger<u64>(data);
}


u32 SaveFile::level(const std::span<u8> data) const {
    return LevelSection.bytesFrom(data).front();
};

}; // namespace savepatcher
