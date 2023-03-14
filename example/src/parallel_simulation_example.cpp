#include <iomanip>
#include <iostream>
#include <mbp/examples/mbp_examples.hpp>
#include <mbp/sim/simulator.hpp>

static constexpr mbp::Batage::TableSpec T00A = {
    .ghistLen = 0, .idxWidth = 14, .tagWidth = 0};
static constexpr mbp::Batage::TableSpec T01A = {
    .ghistLen = 4, .idxWidth = 10, .tagWidth = 7};
static constexpr mbp::Batage::TableSpec T02A = {
    .ghistLen = 5, .idxWidth = 10, .tagWidth = 7};
static constexpr mbp::Batage::TableSpec T03A = {
    .ghistLen = 7, .idxWidth = 10, .tagWidth = 8};
static constexpr mbp::Batage::TableSpec T04A = {
    .ghistLen = 9, .idxWidth = 10, .tagWidth = 8};
static constexpr mbp::Batage::TableSpec T05A = {
    .ghistLen = 13, .idxWidth = 10, .tagWidth = 9};
static constexpr mbp::Batage::TableSpec T06A = {
    .ghistLen = 17, .idxWidth = 10, .tagWidth = 11};
static constexpr mbp::Batage::TableSpec T07A = {
    .ghistLen = 23, .idxWidth = 10, .tagWidth = 11};
static constexpr mbp::Batage::TableSpec T08A = {
    .ghistLen = 32, .idxWidth = 10, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec T09A = {
    .ghistLen = 43, .idxWidth = 10, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec T10A = {
    .ghistLen = 58, .idxWidth = 10, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec T11A = {
    .ghistLen = 78, .idxWidth = 10, .tagWidth = 13};
static constexpr mbp::Batage::TableSpec T12A = {
    .ghistLen = 105, .idxWidth = 10, .tagWidth = 13};
static constexpr mbp::Batage::TableSpec T13A = {
    .ghistLen = 141, .idxWidth = 9, .tagWidth = 17};
static constexpr mbp::Batage::TableSpec T14A = {
    .ghistLen = 191, .idxWidth = 9, .tagWidth = 17};
static constexpr mbp::Batage::TableSpec T15A = {
    .ghistLen = 257, .idxWidth = 9, .tagWidth = 17};

static constexpr mbp::Batage::TableSpec T00B = {
    .ghistLen = 0, .idxWidth = 13, .tagWidth = 0};
static constexpr mbp::Batage::TableSpec T01B = {
    .ghistLen = 4, .idxWidth = 9, .tagWidth = 7};
static constexpr mbp::Batage::TableSpec T02B = {
    .ghistLen = 5, .idxWidth = 9, .tagWidth = 7};
static constexpr mbp::Batage::TableSpec T03B = {
    .ghistLen = 7, .idxWidth = 9, .tagWidth = 8};
static constexpr mbp::Batage::TableSpec T04B = {
    .ghistLen = 9, .idxWidth = 9, .tagWidth = 8};
static constexpr mbp::Batage::TableSpec T05B = {
    .ghistLen = 13, .idxWidth = 9, .tagWidth = 9};
static constexpr mbp::Batage::TableSpec T06B = {
    .ghistLen = 17, .idxWidth = 9, .tagWidth = 11};
static constexpr mbp::Batage::TableSpec T07B = {
    .ghistLen = 23, .idxWidth = 9, .tagWidth = 11};
static constexpr mbp::Batage::TableSpec T08B = {
    .ghistLen = 32, .idxWidth = 9, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec T09B = {
    .ghistLen = 41, .idxWidth = 9, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec T10B = {
    .ghistLen = 51, .idxWidth = 9, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec T11B = {
    .ghistLen = 61, .idxWidth = 9, .tagWidth = 13};
static constexpr mbp::Batage::TableSpec T12B = {
    .ghistLen = 83, .idxWidth = 9, .tagWidth = 13};
static constexpr mbp::Batage::TableSpec T13B = {
    .ghistLen = 105, .idxWidth = 8, .tagWidth = 17};
static constexpr mbp::Batage::TableSpec T14B = {
    .ghistLen = 138, .idxWidth = 8, .tagWidth = 17};
static constexpr mbp::Batage::TableSpec T15B = {
    .ghistLen = 204, .idxWidth = 8, .tagWidth = 17};

static constexpr mbp::Batage::TableSpec T00C = {
    .ghistLen = 0, .idxWidth = 14, .tagWidth = 0};
static constexpr mbp::Batage::TableSpec T01C = {
    .ghistLen = 8, .idxWidth = 9, .tagWidth = 7};
static constexpr mbp::Batage::TableSpec T02C = {
    .ghistLen = 14, .idxWidth = 9, .tagWidth = 7};
static constexpr mbp::Batage::TableSpec T03C = {
    .ghistLen = 20, .idxWidth = 9, .tagWidth = 8};
static constexpr mbp::Batage::TableSpec T04C = {
    .ghistLen = 24, .idxWidth = 10, .tagWidth = 8};
static constexpr mbp::Batage::TableSpec T05C = {
    .ghistLen = 30, .idxWidth = 10, .tagWidth = 9};
static constexpr mbp::Batage::TableSpec T06C = {
    .ghistLen = 34, .idxWidth = 10, .tagWidth = 11};
static constexpr mbp::Batage::TableSpec T07C = {
    .ghistLen = 46, .idxWidth = 10, .tagWidth = 11};
static constexpr mbp::Batage::TableSpec T08C = {
    .ghistLen = 55, .idxWidth = 10, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec T09C = {
    .ghistLen = 66, .idxWidth = 10, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec T10C = {
    .ghistLen = 81, .idxWidth = 10, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec T11C = {
    .ghistLen = 101, .idxWidth = 10, .tagWidth = 13};
static constexpr mbp::Batage::TableSpec T12C = {
    .ghistLen = 128, .idxWidth = 10, .tagWidth = 13};
static constexpr mbp::Batage::TableSpec T13C = {
    .ghistLen = 164, .idxWidth = 10, .tagWidth = 17};
static constexpr mbp::Batage::TableSpec T14C = {
    .ghistLen = 214, .idxWidth = 10, .tagWidth = 17};
static constexpr mbp::Batage::TableSpec T15C = {
    .ghistLen = 280, .idxWidth = 10, .tagWidth = 17};

static constexpr int seed = 158715;

// clang-format off
std::vector<mbp::Predictor*> batages = {
  new mbp::Batage({T00A, T01A, T02A, T03A, T04A, T05A, T06A, T07A,
      T08A, T09A, T10A, T11A, T12A, T13A, T14A, T15A}, seed),
  new mbp::Batage({T00B, T01B, T02B, T03B, T04B, T05B, T06B, T07B,
      T08B, T09B, T10B, T11B, T12B, T13B, T14B, T15B}, seed),
  new mbp::Batage({T00C, T01C, T02C, T03C, T04C, T05C, T06C, T07C,
      T08C, T09C, T10C, T11C, T12C, T13C, T14C, T15C}, seed),
  new mbp::Batage({T00A, T01B, T02A, T03B, T04A, T05B, T06A, T07B,
      T08A, T09B, T10A, T11B, T12A, T13B, T14A, T15B}, seed),
  new mbp::Batage({T00A, T01C, T02A, T03C, T04A, T05C, T06A, T07C,
      T08A, T09C, T10A, T11C, T12A, T13C, T14A, T15C}, seed),
  new mbp::Batage({T00B, T01C, T02B, T03C, T04B, T05C, T06B, T07C,
      T08B, T09C, T10B, T11C, T12B, T13C, T14B, T15C}, seed),
  new mbp::Batage({T00B, T01A, T02B, T03A, T04B, T05A, T06B, T07A,
      T08B, T09A, T10B, T11A, T12B, T13A, T14B, T15A}, seed),
  new mbp::Batage({T00C, T01A, T02C, T03A, T04C, T05A, T06C, T07A,
      T08C, T09A, T10C, T11A, T12C, T13A, T14C, T15A}, seed),
  new mbp::Batage({T00C, T01B, T02C, T03B, T04C, T05B, T06C, T07B,
      T08C, T09B, T10C, T11B, T12C, T13B, T14C, T15B}, seed),
  new mbp::Batage({T00A, T01B, T02C, T03A, T04B, T05C, T06A, T07B,
      T08C, T09A, T10B, T11C, T12A, T13B, T14C, T15A}, seed),
  new mbp::Batage({T00B, T01C, T02A, T03B, T04C, T05A, T06B, T07C,
      T08A, T09B, T10C, T11A, T12B, T13C, T14A, T15B}, seed),
  new mbp::Batage({T00C, T01A, T02B, T03C, T04A, T05B, T06C, T07A,
      T08B, T09C, T10A, T11B, T12C, T13A, T14B, T15C}, seed),
  new mbp::Batage({T00A, T01A, T02B, T03B, T04C, T05C, T06A, T07A,
      T08B, T09B, T10C, T11C, T12A, T13A, T14B, T15B}, seed),
  new mbp::Batage({T00A, T00A, T02A, T02A, T04A, T04A, T06A, T06A,
      T08A, T08A, T10A, T10A, T12A, T12A, T14A, T14A}, seed),
  new mbp::Batage({T00B, T00B, T02B, T02B, T04B, T04B, T06B, T06B,
      T08B, T08B, T10B, T10B, T12B, T12B, T14B, T14B}, seed),
  new mbp::Batage({T00C, T00C, T02C, T02C, T04C, T04C, T06C, T06C,
      T08C, T08C, T10C, T10C, T12C, T12C, T14C, T14C}, seed),
  new mbp::Batage({T00A, T00B, T02A, T02B, T04A, T04B, T06A, T06B,
      T08A, T08B, T10A, T10B, T12A, T12B, T14A, T14B}, seed),
  new mbp::Batage({T00A, T00C, T02A, T02C, T04A, T04C, T06A, T06C,
      T08A, T08C, T10A, T10C, T12A, T12C, T14A, T14C}, seed),
  new mbp::Batage({T00B, T00C, T02B, T02C, T04B, T04C, T06B, T06C,
      T08B, T08C, T10B, T10C, T12B, T12C, T14B, T14C}, seed),
  new mbp::Batage({T00B, T00A, T02B, T02A, T04B, T04A, T06B, T06A,
      T08B, T08A, T10B, T10A, T12B, T12A, T14B, T14A}, seed),
};
// clang-format on

int main(int argc, char** argv) {
  mbp::SimArgs args = mbp::ParseCmdLineArgs(argc, argv);
  batages.resize(NUM_PREDICTORS_TESTED);
  mbp::json output = mbp::ParallelSim(batages, args);
  std::cout << std::setw(2) << output << std::endl;
  return 0;
}
