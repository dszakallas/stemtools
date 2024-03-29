#include <iostream>
#include <fstream>

constexpr uint8_t atomPreambleSize = 8;
constexpr uint8_t extendedSizeFieldSize = 8;

constexpr uint32_t be32(const char* bytes) {
  return ((uint8_t)bytes[0] << 24) | ((uint8_t)bytes[1] << 16) | ((uint8_t)bytes[2] << 8) | (uint8_t)bytes[3];
}

constexpr uint64_t be64(const char* bytes) {
  return ((uint64_t)be32(bytes) << 32) | be32(bytes + 4);
}

constexpr auto moovAtom = be32("moov");
constexpr auto udtaAtom = be32("udta");
constexpr auto stemAtom = be32("stem");

bool read(std::ifstream& stream, char* buffer, uint64_t size) {
  stream.read(buffer, size);
  return stream.gcount() == size;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
    std::cerr << "cannot open file " << argv[1] << std::endl;
    return -1;
  }

  char atomBytes[atomPreambleSize], extendedSizeBytes[extendedSizeFieldSize];
  uint64_t bytesRead, atomSize;
  uint32_t atomType;
  std::vector<uint64_t> openAtoms;

  while (true) {
    if (file.eof() || (openAtoms.size() && !openAtoms.back())) {
      // read file or consumed a child atom without advancing to next level
      std::cerr << "not a stem file" << std::endl;
      return 1;
    }

    // read atom preamble
    if (!read(file, atomBytes, atomPreambleSize)) {
      std::cerr << "failed to read atom preamble" << std::endl;
      return 1;
    }

    bytesRead = atomPreambleSize;
    atomSize = be32(atomBytes);

    if (atomSize == 1) {
      if (!read(file, extendedSizeBytes, extendedSizeFieldSize)) {
        std::cerr << "failed to read extended atom" << std::endl;
        return 1;
      }
      bytesRead += extendedSizeFieldSize;
      atomSize = be64(extendedSizeBytes);
    }
    atomType = be32(atomBytes + 4);

    // commit to consuming the rest of the atom
    if (!openAtoms.empty()) {
      openAtoms.back() -= atomSize;
    }

    atomSize -= bytesRead;

    if (
        openAtoms.size() == 0 && atomType == moovAtom ||
        openAtoms.size() == 1 && atomType == udtaAtom){
      openAtoms.push_back(atomSize);
    } else if (openAtoms.size() == 2 && atomType == stemAtom) {
      std::string stemData(atomSize, 0);
      if (!read(file, stemData.data(), atomSize)) {
        std::cerr << "failed to read stem metadata" << std::endl;
        return 1;
      }
      std::cout << stemData << std::endl;
      break;
    } else {
      file.seekg(atomSize, std::ios::cur);
    }
  }
  return 0;
}

