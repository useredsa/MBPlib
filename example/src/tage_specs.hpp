#ifndef MBPLIB_EXAMPLE_TAGE_SPECS_HPP_
#define MBPLIB_EXAMPLE_TAGE_SPECS_HPP_

#include <mbp/examples/tage.hpp>

// clang-format off
constexpr mbp::Tage::TableSpec T00 =
  {.ghistLen = 0,
   .idxWidth = 14,
   .tagWidth = 0,
   .ctrWidth = 2,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T01 =
  {.ghistLen = 4,
   .idxWidth = 10,
   .tagWidth = 7,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T02 =
  {.ghistLen = 5,
   .idxWidth = 10,
   .tagWidth = 7,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T03 =
  {.ghistLen = 7,
   .idxWidth = 10,
   .tagWidth = 8,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T04 =
  {.ghistLen = 9,
   .idxWidth = 10,
   .tagWidth = 8,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T05 =
  {.ghistLen = 13,
   .idxWidth = 10,
   .tagWidth = 9,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T06 =
  {.ghistLen = 17,
   .idxWidth = 10,
   .tagWidth = 11,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T07 =
  {.ghistLen = 23,
   .idxWidth = 10,
   .tagWidth = 11,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T08 =
  {.ghistLen = 32,
   .idxWidth = 10,
   .tagWidth = 12,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T09 =
  {.ghistLen = 43,
   .idxWidth = 10,
   .tagWidth = 12,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T10 =
  {.ghistLen = 58,
   .idxWidth = 10,
   .tagWidth = 12,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T11 =
  {.ghistLen = 78,
   .idxWidth = 10,
   .tagWidth = 13,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T12 =
  {.ghistLen = 105,
   .idxWidth = 10,
   .tagWidth = 13,
   .ctrWidth = 3,
   .utlWidth = 2};
constexpr mbp::Tage::TableSpec T13 =
  {.ghistLen = 141,
   .idxWidth = 9,
   .tagWidth = 17,
   .ctrWidth = 3,
   .utlWidth = 3};
constexpr mbp::Tage::TableSpec T14 =
  {.ghistLen = 191,
   .idxWidth = 9,
   .tagWidth = 17,
   .ctrWidth = 3,
   .utlWidth = 3};
constexpr mbp::Tage::TableSpec T15 =
  {.ghistLen = 257,
   .idxWidth = 9,
   .tagWidth = 17,
   .ctrWidth = 3,
   .utlWidth = 3};
// clang-format on

constexpr std::array<mbp::Tage::TableSpec, 16> TAGE_SPECS = {
    T00, T01, T02, T03, T04, T05, T06, T07,
    T08, T09, T10, T11, T12, T13, T14, T15};

#endif  // MBPLIB_EXAMPLE_TAGE_SPECS_HPP_
