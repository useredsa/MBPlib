#include "mbp/examples/batage.hpp"
#include "mbp/sim/simulator.hpp"

static constexpr mbp::Batage::TableSpec T00 = {
    .ghistLen = 0, .idxWidth = 14, .tagWidth = 0, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T01 = {
    .ghistLen = 4, .idxWidth = 10, .tagWidth = 7, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T02 = {
    .ghistLen = 5, .idxWidth = 10, .tagWidth = 7, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T03 = {
    .ghistLen = 7, .idxWidth = 10, .tagWidth = 8, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T04 = {
    .ghistLen = 9, .idxWidth = 10, .tagWidth = 8, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T05 = {
    .ghistLen = 13, .idxWidth = 10, .tagWidth = 9, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T06 = {
    .ghistLen = 17, .idxWidth = 10, .tagWidth = 11, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T07 = {
    .ghistLen = 23, .idxWidth = 10, .tagWidth = 11, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T08 = {
    .ghistLen = 32, .idxWidth = 10, .tagWidth = 12, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T09 = {
    .ghistLen = 43, .idxWidth = 10, .tagWidth = 12, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T10 = {
    .ghistLen = 58, .idxWidth = 10, .tagWidth = 12, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T11 = {
    .ghistLen = 78, .idxWidth = 10, .tagWidth = 13, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T12 = {
    .ghistLen = 105, .idxWidth = 10, .tagWidth = 13, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T13 = {
    .ghistLen = 141, .idxWidth = 9, .tagWidth = 17, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T14 = {
    .ghistLen = 191, .idxWidth = 9, .tagWidth = 17, .ctrWidth = 4};
static constexpr mbp::Batage::TableSpec T15 = {
    .ghistLen = 257, .idxWidth = 9, .tagWidth = 17, .ctrWidth = 4};

static constexpr std::array<mbp::Batage::TableSpec, 16> TAGE_SPECS = {
    T00, T01, T02, T03, T04, T05, T06, T07,
    T08, T09, T10, T11, T12, T13, T14, T15};

static constexpr int seed = 158715;

mbp::Batage branchPredictorImpl({TAGE_SPECS.begin(), TAGE_SPECS.end()}, seed);

mbp::Predictor* const mbp::branchPredictor = &branchPredictorImpl;

int main(int argc, char** argv) { return mbp::Simulation(argc, argv); }
