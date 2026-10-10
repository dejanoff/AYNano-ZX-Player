#ifndef AY_PLAYER_BOOKMARK_H
#define AY_PLAYER_BOOKMARK_H
#include <stdint.h>
#include <stddef.h>

struct __attribute__((packed)) PlayerBookmark {
  uint8_t marker;
  uint8_t version;
  uint32_t catalog;
  uint16_t folderIndex, trackIndex, folderCount, trackCount;
  char folder[14];
  char song[13];
  uint8_t checksum;
};

inline uint8_t bookmarkChecksum(const PlayerBookmark& bookmark) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&bookmark);
  uint8_t result = 0;
  for (uint8_t i = 1; i < offsetof(PlayerBookmark, checksum); ++i)
    result = uint8_t((result << 1) | (result >> 7)) ^ bytes[i];
  return result;
}

inline bool bookmarkValid(const PlayerBookmark& b, uint32_t catalog, uint16_t folders) {
  return b.marker == 0xA7 && b.version == 1 && b.catalog == catalog &&
    b.folderCount == folders && b.folderIndex < folders &&
    b.trackCount && b.trackIndex < b.trackCount &&
    b.folder[13] == 0 && b.song[12] == 0 &&
    b.checksum == bookmarkChecksum(b);
}
#endif
