#pragma once
#include "Global.h"
#include "EspFileSystem.h"

extern void writeFileUint8tByFrames(const String& filename, uint8_t*& big_buf, size_t length, size_t headerLenth, size_t frameSize);
extern void writeFileUint8tByByte(const String& filename, uint8_t*& payload, size_t length, size_t headerLenth);
extern File seekFile(const String& filename, size_t position = 0);
extern const String writeFile(const String& filename, const String& str);
extern const String readFile(const String& filename, size_t max_size);
extern const String filepath(const String& filename);
extern bool cutFile(const String& src, const String& dst);
extern const String addFileLn(const String& filename, const String& str);
extern void onFlashWrite();