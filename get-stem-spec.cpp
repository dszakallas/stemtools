#include <iostream>
#include <fstream>

constexpr uint8_t atomPreambleSize = 8;
constexpr uint8_t extendedSizeFieldSize = 8;

constexpr uint32_t be32(const char* bytes) {
  return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

constexpr uint64_t be64(const char* bytes) {
  return ((uint64_t)be32(bytes) << 32) | be32(bytes + 4);
}

constexpr auto moovAtom = be32("moov");
constexpr auto udtaAtom = be32("udta");
constexpr auto stemAtom = be32("stem");

std::string _atomTypeToString(uint32_t atomType) {
  std::string result;
  result.push_back((atomType >> 24) & 0xff);
  result.push_back((atomType >> 16) & 0xff);
  result.push_back((atomType >> 8) & 0xff);
  result.push_back(atomType & 0xff);
  return result;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
    std::cerr << "Cannot open file " << argv[1] << std::endl;
    return -1;
  }

  std::vector<uint64_t> openAtoms;

  while (true) {
    if (file.eof()) {
      break;
    }
    if (!openAtoms.empty() && !openAtoms.back()) {
      openAtoms.pop_back();
      continue;
    }

    // read atom preamble
    char atomBytes[atomPreambleSize];
    file.read(atomBytes, atomPreambleSize);
    if (file.gcount() != atomPreambleSize) {
      break;
    }
    uint64_t bytesRead = 0;
    bytesRead += atomPreambleSize;

    uint64_t atomSize = be32(atomBytes);

    if (atomSize == 1) {
      std::cerr << "Extended size field" << std::endl;
      char extendedSizeBytes[extendedSizeFieldSize];
      file.read(extendedSizeBytes, extendedSizeFieldSize);
      if (file.gcount() != extendedSizeFieldSize) {
        break;
      }
      bytesRead += extendedSizeFieldSize;
      atomSize = be64(extendedSizeBytes);
    }
    uint32_t atomType = be32(atomBytes + 4);

    // process atom
    for (int i = 0; i < openAtoms.size(); i++) {
      std::cerr << "\t";
    }
    std::cerr << _atomTypeToString(atomType) << "[" << atomSize << "]" << std::endl;

    // commit to consuming the rest of the atom
    if (!openAtoms.empty()) {
      openAtoms.back() -= atomSize;
    }

    if (
        !openAtoms.size() && atomType == moovAtom ||
        openAtoms.size() == 1 && atomType == udtaAtom){
      openAtoms.push_back(atomSize - bytesRead);
    } else if (openAtoms.size() == 2 && atomType == stemAtom) {
      char stemData[atomSize - bytesRead];
      file.read(stemData, atomSize - bytesRead);
      if (file.gcount() != atomSize - bytesRead) {
        break;
      }
      std::cout << std::string(stemData, atomSize - bytesRead) << std::endl;
    } else {
      file.seekg(atomSize - bytesRead, std::ios::cur);
    }
  }
  return 0;
}

