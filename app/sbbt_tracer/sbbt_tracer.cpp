#include <unistd.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "pin.H"
#include "instlib.H"

// Command Line Arguments ─────────────────────────────────────────────────────

static std::string traceFile;
static std::string tmpFile;
static std::string detailedInfoFile;
static uint64_t instrToSkip;
static uint64_t instrToProbe;
static bool exitEarly;

static KNOB<std::string> traceFileKnob(KNOB_MODE_WRITEONCE,
    "pintool",
    "o",
    "trace",
    "Output trace file basename");
static KNOB<std::string> detailedInfoFileKnob(KNOB_MODE_WRITEONCE,
    "pintool",
    "details-file",
    "",
    "Output log file with mnemonics basename. "
    "If not set, the file is not created");
static KNOB<std::string> instrToSkipKnob(KNOB_MODE_WRITEONCE,
    "pintool",
    "skip",
    "0",
    "Number of instructions to skip before starting tracing");
static KNOB<std::string> instrToProbeKnob(KNOB_MODE_WRITEONCE,
    "pintool",
    "length",
    "0",
    "Number of instructions probed, 0 means the whole program");
static KNOB<std::string> exitEarlyKnob(KNOB_MODE_WRITEONCE,
    "pintool",
    "early-exit",
    "true",
    "Exit the program early if the probing is finished");

static void ParseCmdLineArgs(int argc, char** argv) {
  if (int return_code; (return_code = PIN_Init(argc, argv))) {
    std::cerr << "Usage: \"$PIN_ROOT/pin\" [pin options...] -t sbbt_tracer.so "
                 "[sbbt_tracer options...]\n";
    std::cerr << KNOB_BASE::StringKnobSummary() << std::flush;
    std::exit(return_code);
  }

  traceFile = traceFileKnob.Value() + ".sbbt.zst";
  tmpFile = traceFile + ".tmp";
  detailedInfoFile = detailedInfoFileKnob.Value() == ""
                         ? ""
                         : detailedInfoFileKnob.Value() + ".zst";

  char* endptr;
  if ((instrToProbe = strtoll(instrToProbeKnob.Value().c_str(), &endptr, 0)) <
      0) {
    std::cerr << "sbbt_tracer: <instr_to_trace> cannot be negative\n";
    std::exit(1);
  }
  if (endptr == instrToProbeKnob.Value().c_str()) {
    std::cerr << "sbbt_tracer: '" << instrToProbeKnob.Value()
              << "' is not a valid number of instructions to probe\n";
    std::exit(1);
  }
  if ((instrToSkip = strtoll(instrToSkipKnob.Value().c_str(), &endptr, 0)) <
      0) {
    std::cerr << "sbbt_tracer: <instr_to_trace> cannot be negative\n";
    std::exit(1);
  }
  if (endptr == instrToSkipKnob.Value().c_str()) {
    std::cerr << "sbbt_tracer: '" << instrToSkipKnob.Value()
              << "' is not a valid number of instructions to skip\n";
    std::exit(1);
  }
  if (errno != 0) {
    std::cerr << std::strerror(errno) << std::endl;
    exit(1);
  }

  exitEarly = exitEarlyKnob.Value() == "true";
  if (exitEarlyKnob.Value() != "true" && exitEarlyKnob.Value() != "false") {
    std::cerr << "sbbt_tracer: '" << exitEarlyKnob.Value()
              << "' is not a valid value (expected true or false)\n";
    std::exit(1);
  }
}

// Output Generation ──────────────────────────────────────────────────────────

static constexpr const char* COMPRESSION_CMD = "zstd --ultra -22 --force -o ";
static constexpr const char* DECOMPRESSION_CMD = "zstd --decompress --stdout ";

enum OpCode {
  // Number of possible opcodes.
  NUMBER = 0b10000,
  // Bitmask corresponding to the base type.
  TYPE = 0b1100,
  // Bit for conditional branches.
  CND = 0b0001,
  // Bit for indirect branches.
  IND = 0b0010,
  // JUMP base type.
  JUMP = 0b0000,
  // RET (return from function) base type.
  RET = 0b0100,
  // CALL (function) base type.
  CALL = 0b1000,
};

struct SbbtHeader {
  char s = 'S';
  char b0 = 'B';
  char b1 = 'B';
  char t = 'T';
  char nl = '\n';
  uint8_t vmajor = 1;
  uint8_t vminor = 0;
  uint8_t vpatch = 0;
  uint64_t numInstructions;
  uint64_t numBranches;
};

struct SbbtBranch {
  unsigned opcode : 4;
  unsigned padding : 7;
  unsigned outcome : 1;
  unsigned long long ip : 52;
  unsigned ninstr : 12;
  unsigned long long target : 52;
};

static_assert(sizeof(SbbtHeader) == 24);
static_assert(sizeof(SbbtBranch) == 16);

static uint64_t sign_unextend_ip(uint64_t ip) {
  constexpr uint64_t last_and_mask = ((1ULL << 13) - 1) << 51;
  constexpr uint64_t mask = ((1ULL << 12) - 1) << 52;
  assert((ip & last_and_mask) == last_and_mask || (ip & last_and_mask) == 0);
  return ip & ~mask;
}

static FILE* trace;
static FILE* detailedInfo;
static uint64_t numInstructions;
static uint64_t numBranches;
static uint64_t lastBranch;

void OpenOutputFiles() {
  string cmd = COMPRESSION_CMD + tmpFile;
  trace = popen(cmd.c_str(), "w");
  if (trace == nullptr) {
    std::cerr << std::strerror(errno) << std::endl;
    exit(2);
  }
  if (!detailedInfoFile.empty()) {
    cmd = COMPRESSION_CMD + detailedInfoFile;
    detailedInfo = popen(cmd.c_str(), "w");
    if (detailedInfo == nullptr) {
      std::cerr << std::strerror(errno) << std::endl;
      exit(2);
    }
  }

  lastBranch = instrToSkip;
}

void RecompressTraceFile(int, void*) {
  if (pclose(trace) == -1) {
    LOG(std::string(std::strerror(errno)) + "\n");
    exit(4);
  }
  if (detailedInfo != nullptr && pclose(detailedInfo) == -1) {
    std::cerr << std::strerror(errno) << std::endl;
    exit(4);
  }
  LOG("Finished probing program.\n"
      "\tStopped after " +
      std::to_string(numInstructions) + " instructions\n\tProbed: " +
      std::to_string(numBranches) + " branches\n");

  LOG("Recompressing file with the header\n");
  std::string cmd = DECOMPRESSION_CMD + tmpFile;
  FILE* tmp = popen(cmd.c_str(), "r");
  if (tmp == nullptr) {
    LOG(std::string(std::strerror(errno)) + "\n");
    exit(4);
  }
  cmd = COMPRESSION_CMD + traceFile;
  FILE* out = popen(cmd.c_str(), "w");
  if (out == nullptr) {
    LOG(std::string(std::strerror(errno)) + "\n");
    exit(4);
  }

  SbbtHeader sbbtHeader;
  sbbtHeader.numInstructions =
      instrToProbe != 0 ? instrToProbe : numInstructions;
  sbbtHeader.numBranches = numBranches;
  fwrite(&sbbtHeader, sizeof(sbbtHeader), 1, out);
  if (std::ferror(out)) {
    LOG(std::string(std::strerror(errno)) + "\n");
    exit(4);
  }

  static constexpr size_t READ_SIZE = 1 << 16;
  std::array<char, READ_SIZE> buffer;
  size_t read = 0;
  while ((read = fread(buffer.data(), 1, READ_SIZE, tmp)) != 0) {
    fwrite(buffer.data(), 1, read, out);
    if (std::ferror(tmp) || std::ferror(out)) {
      LOG(std::string(std::strerror(errno)) + "\n");
      exit(4);
    }
  }
  if (std::ferror(tmp) || std::ferror(out)) {
    LOG(std::string(std::strerror(errno)) + "\n");
    exit(4);
  }
  if (pclose(tmp) == -1) {
    LOG(std::string(std::strerror(errno)) + "\n");
    exit(4);
  }
  if (pclose(out) == -1) {
    LOG(std::string(std::strerror(errno)) + "\n");
    exit(4);
  }
  unlink(tmpFile.c_str());
}

// Instrumentation and Probing ─────────────────────────────────────────────────

static void IncrementNumInstructions() { numInstructions += 1; }

static char* Mnemonic(uint64_t ip) {
  static char buffer[128];
  memset(buffer, 0, sizeof(buffer));

  xed_state_t dstate;
  if (sizeof(ADDRINT) == 4) {
    xed_state_init2(&dstate, XED_MACHINE_MODE_LEGACY_32, XED_ADDRESS_WIDTH_32b);
  } else {
    xed_state_init2(&dstate, XED_MACHINE_MODE_LONG_64, XED_ADDRESS_WIDTH_64b);
  }
  xed_decoded_inst_t xedd;
  xed_decoded_inst_zero_set_mode(&xedd, &dstate);
  static constexpr uint32_t MAX_INSTRUCTION_SIZE = 15;
  xed_error_enum_t decodeResult = xed_decode(
      &xedd, reinterpret_cast<const uint8_t*>(ip), MAX_INSTRUCTION_SIZE);
  if (decodeResult != XED_ERROR_NONE) {
    LOG("Error decoding ip " + std::to_string(ip) + "\n");
  } else {
    int formatResult = xed_format_context(
        XED_SYNTAX_INTEL, &xedd, buffer, sizeof(buffer), ip, 0, 0);
    if (formatResult == 0) {
      LOG("Error disassembling ip " + std::to_string(ip) + "\n");
    }
  }
  return buffer;
}

static void RecordControlFlowInstr(uint32_t opcode,
    uint64_t ip,
    uint64_t target,
    bool taken) {
  if (numInstructions < instrToSkip) return;
  if (instrToProbe != 0 && numInstructions >= instrToSkip + instrToProbe) {
    if (!exitEarly) return;
    RecompressTraceFile(0, 0);
    exit(0);
  }
  SbbtBranch binarybranch;
  binarybranch.outcome = taken;
  binarybranch.padding = 0;
  binarybranch.opcode = opcode;
  binarybranch.ip = sign_unextend_ip(ip);
  binarybranch.ninstr = numInstructions - lastBranch;
  binarybranch.target = sign_unextend_ip(target);
  fwrite(&binarybranch, sizeof(binarybranch), 1, trace);
  if (std::ferror(trace)) {
    LOG(std::string(std::strerror(errno)) + "\n");
    exit(3);
  }
  lastBranch = numInstructions;
  numBranches += 1;

  if (detailedInfo == nullptr) return;
  std::ostringstream os;
  os << std::setw(11) << std::setfill(' ') << numInstructions - instrToSkip
     << " 0x" << std::hex << std::setw(16) << std::setfill('0') << ip << " 0x"
     << std::setw(16) << std::setfill('0') << target << std::dec
     << (opcode & IND ? " IND" : " DIR") << (opcode & CND ? " CND" : " UCD")
     << ((opcode & TYPE) == JUMP
                ? " JUMP"
                : ((opcode & TYPE) == CALL ? " CALL" : " RET "))
     << (taken ? " T \"" : " N \"") << Mnemonic(ip) << "\"\n";
  std::string line = os.str();
  fwrite(line.c_str(), 1, line.size(), detailedInfo);
  if (std::ferror(trace)) {
    LOG(std::string(std::strerror(errno)) + "\n");
    exit(3);
  }
}

static void GeneralInstrumentation(INS ins, void* v) {
  INS_InsertCall(
      ins, IPOINT_BEFORE, (AFUNPTR)IncrementNumInstructions, IARG_END);
  if (!INS_IsControlFlow(ins)) return;
  uint32_t opcode = JUMP;
  if (INS_IsCall(ins)) {
    opcode = CALL;
  } else if (INS_IsRet(ins)) {
    opcode = RET;
  }
  if (INS_HasFallThrough(ins)) {
    opcode = opcode | CND;
  }
  if (!INS_IsDirectControlFlow(ins)) {
    opcode = opcode | IND;
  }
  INS_InsertCall(ins,
      IPOINT_BEFORE,
      (AFUNPTR)RecordControlFlowInstr,
      IARG_UINT32,
      opcode,
      IARG_INST_PTR,
      IARG_BRANCH_TARGET_ADDR,
      IARG_BRANCH_TAKEN,
      IARG_END);
}

int main(int argc, char** argv) {
  ParseCmdLineArgs(argc, argv);
  OpenOutputFiles();
  PIN_AddFiniFunction(RecompressTraceFile, 0);
  INS_AddInstrumentFunction(GeneralInstrumentation, 0);
  PIN_StartProgram();
}
