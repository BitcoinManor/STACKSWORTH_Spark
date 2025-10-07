#pragma once
#include <stdint.h>

// Simple color helper (RGB hex)
#define AXE_RGB(hex) (uint32_t)(hex)

namespace axe {

// ---------------- Colors ----------------
namespace color {
  static const uint32_t bg    = AXE_RGB(0x0C0F14);
  static const uint32_t card  = AXE_RGB(0x141A22);
  static const uint32_t text  = AXE_RGB(0xFFFFFF);
  static const uint32_t muted = AXE_RGB(0x9AA4B2);
  static const uint32_t accent= AXE_RGB(0xA0FFC8);
  static const uint32_t warn  = AXE_RGB(0xFFCC66);
  static const uint32_t error = AXE_RGB(0xFF6B6B);
} // namespace color

// Brand accent(s)
namespace brand {
  static const uint32_t orange = AXE_RGB(0xFCA420);  // classic STACKSWORTH orange
} // namespace brand

// ---------------- Spacing ----------------
namespace spacing {
  static const int xs = 4;
  static const int sm = 8;
  static const int md = 16;
  static const int lg = 24;
  static const int xl = 32;
} // namespace spacing

// ---------------- Radii ----------------
namespace radii {
  static const int card = 12;
  static const int pill = 999;
} // namespace radii

// ---------------- Type (px) ----------------
namespace type {
  static const int h1   = 28;
  static const int h2   = 22;
  static const int body = 18;
  static const int mono = 16;
} // namespace type

} // namespace axe
