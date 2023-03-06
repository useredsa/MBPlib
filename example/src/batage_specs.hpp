#ifndef MBPLIB_EXAMPLE_BATAGE_SPECS_HPP_
#define MBPLIB_EXAMPLE_BATAGE_SPECS_HPP_

#include <mbp/examples/batage.hpp>
#include <mbp/sim/simulator.hpp>

static constexpr mbp::Batage::TableSpec BT00 = {
    .ghistLen = 0, .idxWidth = 14, .tagWidth = 0};
static constexpr mbp::Batage::TableSpec BT01 = {
    .ghistLen = 4, .idxWidth = 10, .tagWidth = 7};
static constexpr mbp::Batage::TableSpec BT02 = {
    .ghistLen = 5, .idxWidth = 10, .tagWidth = 7};
static constexpr mbp::Batage::TableSpec BT03 = {
    .ghistLen = 7, .idxWidth = 10, .tagWidth = 8};
static constexpr mbp::Batage::TableSpec BT04 = {
    .ghistLen = 9, .idxWidth = 10, .tagWidth = 8};
static constexpr mbp::Batage::TableSpec BT05 = {
    .ghistLen = 13, .idxWidth = 10, .tagWidth = 9};
static constexpr mbp::Batage::TableSpec BT06 = {
    .ghistLen = 17, .idxWidth = 10, .tagWidth = 11};
static constexpr mbp::Batage::TableSpec BT07 = {
    .ghistLen = 23, .idxWidth = 10, .tagWidth = 11};
static constexpr mbp::Batage::TableSpec BT08 = {
    .ghistLen = 32, .idxWidth = 10, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec BT09 = {
    .ghistLen = 43, .idxWidth = 10, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec BT10 = {
    .ghistLen = 58, .idxWidth = 10, .tagWidth = 12};
static constexpr mbp::Batage::TableSpec BT11 = {
    .ghistLen = 78, .idxWidth = 10, .tagWidth = 13};
static constexpr mbp::Batage::TableSpec BT12 = {
    .ghistLen = 105, .idxWidth = 10, .tagWidth = 13};
static constexpr mbp::Batage::TableSpec BT13 = {
    .ghistLen = 141, .idxWidth = 9, .tagWidth = 17};
static constexpr mbp::Batage::TableSpec BT14 = {
    .ghistLen = 191, .idxWidth = 9, .tagWidth = 17};
static constexpr mbp::Batage::TableSpec BT15 = {
    .ghistLen = 257, .idxWidth = 9, .tagWidth = 17};

static constexpr std::array<mbp::Batage::TableSpec, 16> BATAGE_SPECS = {
    BT00, BT01, BT02, BT03, BT04, BT05, BT06, BT07,
    BT08, BT09, BT10, BT11, BT12, BT13, BT14, BT15};

static constexpr int BATAGE_SEED = 158715;

#endif  // MBPLIB_EXAMPLE_TAGE_SPECS_HPP_
