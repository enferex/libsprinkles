#include "sprinkles.hh"

#include <llvm/ADT/StringExtras.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCTargetOptions.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>

#include <algorithm>
#include <cassert>
#include <llvm/CodeGen/CommandFlags.inc>
using namespace llvm;
using namespace sprinkles;

static bool _initialized;

Sprinkles::Sprinkles(const char *fname) : _inputFname(fname) {
  if (!_initialized) {
    InitializeAllTargetInfos();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllDisassemblers();
    _initialized = true;
  }
}

const std::vector<MCInst> &Sprinkles::getInstructions() const {
  return _instructions;
}

const std::vector<object::SymbolRef> &Sprinkles::getSymbols() const {
  return _symbols;
}

const std::vector<object::RelocationRef> &Sprinkles::getRelocs() const {
  return _relocations;
}

Error Sprinkles::initialize() { return parseObject(); }

const InstList Sprinkles::getInstructions(const object::SymbolRef &sr) const {
  auto typeOrErr = sr.getType();
  if (!typeOrErr) return {};
  if (*typeOrErr == object::SymbolRef::ST_Function) return {};

  // Only get instructions for function bodies (only .text).
  auto sectOrErr = sr.getSection();
  if (!sectOrErr) return {};
  const object::SectionRef &sect = **sectOrErr;
  if (!sect.isText()) return {};
  auto pr = _sectionToInsns.find(sect.getIndex());
  if (pr == _sectionToInsns.end()) return {};
  return InstList(pr->second.first, pr->second.second);
}

const object::ObjectFile *Sprinkles::getObjectFile() const {
  return _objFile.get();
}

Error Sprinkles::parseObject() {
  // Load the object file and obtain its buffer.
  auto owner = object::ObjectFile::createObjectFile(_inputFname);
  if (!owner) return owner.takeError();
  auto pr = owner->takeBinary();
  _objFile = std::move(pr.first);
  _objBuffer = std::move(pr.second);

  // Obtain the target data from the object file.
  // Special thanks for llvm-objdump for outlining this process.
  Triple triple = _objFile->makeTriple();
  std::string err;
  const Target *target = TargetRegistry::lookupTarget(MArch, triple, err);
  if (!target) {
    return make_error<StringError>(
        "Error identifying the target for the input object file: " + err + '\n',
        inconvertibleErrorCode());
  }

  // Create a disassembler (most of this setup is from llvm-objdump).
  _sti = std::unique_ptr<const MCSubtargetInfo>(target->createMCSubtargetInfo(
      triple.getTriple(), "", _objFile->getFeatures().getString()));
  const std::string tt = triple.getTriple();
  _rinfo = std::unique_ptr<const MCRegisterInfo>(target->createMCRegInfo(tt));
  _ainfo = std::unique_ptr<const MCAsmInfo>(
      target->createMCAsmInfo(*_rinfo, tt, MCTargetOptions()));
  _iinfo = std::unique_ptr<const MCInstrInfo>(target->createMCInstrInfo());
  _printer = std::unique_ptr<MCInstPrinter>(target->createMCInstPrinter(
      triple, 0, *_ainfo.get(), *_iinfo.get(), *_rinfo.get()));
  MCObjectFileInfo oinfo;
  MCContext ctx(_ainfo.get(), _rinfo.get(), &oinfo);
  oinfo.InitMCObjectFileInfo(triple, false, ctx);
  std::unique_ptr<MCDisassembler> dis(target->createMCDisassembler(*_sti, ctx));

  // Keep track of the current and last iterator into _instructions.
  auto firstSectionInstruction = _instructions.cend();

  // Disassemble!
  for (const object::SectionRef &sect : _objFile->sections()) {
    if (!sect.isText()) continue;
    auto bytesOrErr = sect.getContents();
    if (!bytesOrErr) return bytesOrErr.takeError();
    ArrayRef<uint8_t> bytes = arrayRefFromStringRef(*bytesOrErr);
    MCInst inst;
    const uint64_t base = sect.getAddress();
    const uint64_t maxSize = std::min(sect.getSize(), bytes.size());
    uint64_t size = 0, offset = 0, nAdded = 0;
    while (offset < maxSize) {
      MCDisassembler::DecodeStatus status = dis->getInstruction(
          inst, size, bytes.slice(offset), base + offset, nulls());
      if (status == MCDisassembler::Success) {
        nAdded++;
        _instructions.push_back(inst);
      }
      offset += (size == 0 ? 1 : size);
    }

    // Keep track of the instructions we just added.
    // The first instruction was originally the end of the last section so we
    // have to +1 so that it points to the first insn of this section.
    // The last instruction for this section is is first + nAdded - 1
    if (nAdded > 0) {
      ++firstSectionInstruction;
      auto pr = std::make_pair(firstSectionInstruction,
                               firstSectionInstruction + nAdded - 1);
      _sectionToInsns.emplace(std::make_pair(sect.getIndex(), pr));
    }
  }

  // Collect the symbols.
  for (const object::SymbolRef &sr : _objFile->symbols())
    _symbols.push_back(sr);

  // Collect the relocations.
  for (const object::SectionRef &sect : _objFile->dynamic_relocation_sections())
    for (const object::RelocationRef &rr : sect.relocations())
      _relocations.push_back(rr);

  return Error::success();
}

const MCInstrDesc &Sprinkles::getDesc(const MCInst &mi) const {
  return _iinfo->get(mi.getOpcode());
}

MCInstPrinter *Sprinkles::getPrinter() const { return _printer.get(); }

void Sprinkles::dump(const MCInst *mi) const {
  _printer->printInst(mi, 0, "", *_sti, outs());
}
