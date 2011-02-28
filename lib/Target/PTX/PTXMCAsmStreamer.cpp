//===- lib/Target/PTX/PTXMCAsmStreamer.cpp - PTX Text Assembly Output -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PTXMCAsmStreamer.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Format.h"

using namespace llvm;

/// TODO: Add appropriate implementation of Emit*() methods when needed

void PTXMCAsmStreamer::AddComment(const Twine &T) {
  if (!IsVerboseAsm) return;

  // Make sure that CommentStream is flushed.
  CommentStream.flush();

  T.toVector(CommentToEmit);
  // Each comment goes on its own line.
  CommentToEmit.push_back('\n');

  // Tell the comment stream that the vector changed underneath it.
  CommentStream.resync();
}

void PTXMCAsmStreamer::EmitCommentsAndEOL() {
  if (CommentToEmit.empty() && CommentStream.GetNumBytesInBuffer() == 0) {
    OS << '\n';
    return;
  }

  CommentStream.flush();
  StringRef Comments = CommentToEmit.str();

  assert(Comments.back() == '\n' &&
         "Comment array not newline terminated");
  do {
    // Emit a line of comments.
    OS.PadToColumn(MAI.getCommentColumn());
    size_t Position = Comments.find('\n');
    OS << MAI.getCommentString() << ' ' << Comments.substr(0, Position) << '\n';

    Comments = Comments.substr(Position+1);
  } while (!Comments.empty());

  CommentToEmit.clear();
  // Tell the comment stream that the vector changed underneath it.
  CommentStream.resync();
}

static inline int64_t truncateToSize(int64_t Value, unsigned Bytes) {
  assert(Bytes && "Invalid size!");
  return Value & ((uint64_t) (int64_t) -1 >> (64 - Bytes * 8));
}

void PTXMCAsmStreamer::SwitchSection(const MCSection *Section) {
  assert(Section && "Cannot switch to a null section!");
  if (Section != CurSection) {
    PrevSection = CurSection;
    CurSection = Section;
  }
}

void PTXMCAsmStreamer::EmitLabel(MCSymbol *Symbol) {
  assert(Symbol->isUndefined() && "Cannot define a symbol twice!");
  assert(!Symbol->isVariable() && "Cannot emit a variable symbol!");
  assert(CurSection && "Cannot emit before setting section!");

  OS << *Symbol << MAI.getLabelSuffix();
  EmitEOL();
  Symbol->setSection(*CurSection);
}

void PTXMCAsmStreamer::EmitAssemblerFlag(MCAssemblerFlag Flag) {}

void PTXMCAsmStreamer::EmitThumbFunc(MCSymbol *Func) {}

void PTXMCAsmStreamer::EmitAssignment(MCSymbol *Symbol, const MCExpr *Value) {
  OS << *Symbol << " = " << *Value;
  EmitEOL();

  // FIXME: Lift context changes into super class.
  Symbol->setVariableValue(Value);
}

void PTXMCAsmStreamer::EmitWeakReference(MCSymbol *Alias,
                                         const MCSymbol *Symbol) {
  OS << ".weakref " << *Alias << ", " << *Symbol;
  EmitEOL();
}

void PTXMCAsmStreamer::EmitSymbolAttribute(MCSymbol *Symbol,
                                           MCSymbolAttr Attribute) {}

void PTXMCAsmStreamer::EmitSymbolDesc(MCSymbol *Symbol, unsigned DescValue) {}

void PTXMCAsmStreamer::BeginCOFFSymbolDef(const MCSymbol *Symbol) {}

void PTXMCAsmStreamer::EmitCOFFSymbolStorageClass (int StorageClass) {}

void PTXMCAsmStreamer::EmitCOFFSymbolType (int Type) {}

void PTXMCAsmStreamer::EndCOFFSymbolDef() {}

void PTXMCAsmStreamer::EmitELFSize(MCSymbol *Symbol, const MCExpr *Value) {}

void PTXMCAsmStreamer::EmitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                        unsigned ByteAlignment) {}

void PTXMCAsmStreamer::EmitLocalCommonSymbol(MCSymbol *Symbol, uint64_t Size) {}

void PTXMCAsmStreamer::EmitZerofill(const MCSection *Section, MCSymbol *Symbol,
                                    unsigned Size, unsigned ByteAlignment) {}

void PTXMCAsmStreamer::EmitTBSSSymbol(const MCSection *Section,
                                      MCSymbol *Symbol,
                                      uint64_t Size, unsigned ByteAlignment) {}

static inline char toOctal(int X) { return (X&7)+'0'; }

static void PrintQuotedString(StringRef Data, raw_ostream &OS) {
  OS << '"';

  for (unsigned i = 0, e = Data.size(); i != e; ++i) {
    unsigned char C = Data[i];
    if (C == '"' || C == '\\') {
      OS << '\\' << (char)C;
      continue;
    }

    if (isprint((unsigned char)C)) {
      OS << (char)C;
      continue;
    }

    switch (C) {
      case '\b': OS << "\\b"; break;
      case '\f': OS << "\\f"; break;
      case '\n': OS << "\\n"; break;
      case '\r': OS << "\\r"; break;
      case '\t': OS << "\\t"; break;
      default:
        OS << '\\';
        OS << toOctal(C >> 6);
        OS << toOctal(C >> 3);
        OS << toOctal(C >> 0);
        break;
    }
  }

  OS << '"';
}

void PTXMCAsmStreamer::EmitBytes(StringRef Data, unsigned AddrSpace) {
  assert(CurSection && "Cannot emit contents before setting section!");
  if (Data.empty()) return;

  if (Data.size() == 1) {
    OS << MAI.getData8bitsDirective(AddrSpace);
    OS << (unsigned)(unsigned char)Data[0];
    EmitEOL();
    return;
  }

  // If the data ends with 0 and the target supports .asciz, use it, otherwise
  // use .ascii
  if (MAI.getAscizDirective() && Data.back() == 0) {
    OS << MAI.getAscizDirective();
    Data = Data.substr(0, Data.size()-1);
  } else {
    OS << MAI.getAsciiDirective();
  }

  OS << ' ';
  PrintQuotedString(Data, OS);
  EmitEOL();
}

/// EmitIntValue - Special case of EmitValue that avoids the client having
/// to pass in a MCExpr for constant integers.
void PTXMCAsmStreamer::EmitIntValue(uint64_t Value, unsigned Size,
                                    unsigned AddrSpace) {
  assert(CurSection && "Cannot emit contents before setting section!");
  const char *Directive = 0;
  switch (Size) {
  default: break;
  case 1: Directive = MAI.getData8bitsDirective(AddrSpace); break;
  case 2: Directive = MAI.getData16bitsDirective(AddrSpace); break;
  case 4: Directive = MAI.getData32bitsDirective(AddrSpace); break;
  case 8:
    Directive = MAI.getData64bitsDirective(AddrSpace);
    // If the target doesn't support 64-bit data, emit as two 32-bit halves.
    if (Directive) break;
    if (isLittleEndian()) {
      EmitIntValue((uint32_t)(Value >> 0 ), 4, AddrSpace);
      EmitIntValue((uint32_t)(Value >> 32), 4, AddrSpace);
    } else {
      EmitIntValue((uint32_t)(Value >> 32), 4, AddrSpace);
      EmitIntValue((uint32_t)(Value >> 0 ), 4, AddrSpace);
    }
    return;
  }

  assert(Directive && "Invalid size for machine code value!");
  OS << Directive << truncateToSize(Value, Size);
  EmitEOL();
}

void PTXMCAsmStreamer::EmitValue(const MCExpr *Value, unsigned Size,
                                 unsigned AddrSpace) {
  assert(CurSection && "Cannot emit contents before setting section!");
  const char *Directive = 0;
  switch (Size) {
  default: break;
  case 1: Directive = MAI.getData8bitsDirective(AddrSpace); break;
  case 2: Directive = MAI.getData16bitsDirective(AddrSpace); break;
  case 4: Directive = MAI.getData32bitsDirective(AddrSpace); break;
  case 8: Directive = MAI.getData64bitsDirective(AddrSpace); break;
  }

  assert(Directive && "Invalid size for machine code value!");
  OS << Directive << *Value;
  EmitEOL();
}

void PTXMCAsmStreamer::EmitULEB128Value(const MCExpr *Value,
                                        unsigned AddrSpace) {
  assert(MAI.hasLEB128() && "Cannot print a .uleb");
  OS << ".uleb128 " << *Value;
  EmitEOL();
}

void PTXMCAsmStreamer::EmitSLEB128Value(const MCExpr *Value,
                                        unsigned AddrSpace) {
  assert(MAI.hasLEB128() && "Cannot print a .sleb");
  OS << ".sleb128 " << *Value;
  EmitEOL();
}

void PTXMCAsmStreamer::EmitGPRel32Value(const MCExpr *Value) {
  assert(MAI.getGPRel32Directive() != 0);
  OS << MAI.getGPRel32Directive() << *Value;
  EmitEOL();
}


/// EmitFill - Emit NumBytes bytes worth of the value specified by
/// FillValue.  This implements directives such as '.space'.
void PTXMCAsmStreamer::EmitFill(uint64_t NumBytes, uint8_t FillValue,
                                unsigned AddrSpace) {
  if (NumBytes == 0) return;

  if (AddrSpace == 0)
    if (const char *ZeroDirective = MAI.getZeroDirective()) {
      OS << ZeroDirective << NumBytes;
      if (FillValue != 0)
        OS << ',' << (int)FillValue;
      EmitEOL();
      return;
    }

  // Emit a byte at a time.
  MCStreamer::EmitFill(NumBytes, FillValue, AddrSpace);
}

void PTXMCAsmStreamer::EmitValueToAlignment(unsigned ByteAlignment, int64_t Value,
                                            unsigned ValueSize,
                                            unsigned MaxBytesToEmit) {
  // Some assemblers don't support non-power of two alignments, so we always
  // emit alignments as a power of two if possible.
  if (isPowerOf2_32(ByteAlignment)) {
    switch (ValueSize) {
    default: llvm_unreachable("Invalid size for machine code value!");
    case 1: OS << MAI.getAlignDirective(); break;
    // FIXME: use MAI for this!
    case 2: OS << ".p2alignw "; break;
    case 4: OS << ".p2alignl "; break;
    case 8: llvm_unreachable("Unsupported alignment size!");
    }

    if (MAI.getAlignmentIsInBytes())
      OS << ByteAlignment;
    else
      OS << Log2_32(ByteAlignment);

    if (Value || MaxBytesToEmit) {
      OS << ", 0x";
      OS.write_hex(truncateToSize(Value, ValueSize));

      if (MaxBytesToEmit)
        OS << ", " << MaxBytesToEmit;
    }
    EmitEOL();
    return;
  }

  // Non-power of two alignment.  This is not widely supported by assemblers.
  // FIXME: Parameterize this based on MAI.
  switch (ValueSize) {
  default: llvm_unreachable("Invalid size for machine code value!");
  case 1: OS << ".balign";  break;
  case 2: OS << ".balignw"; break;
  case 4: OS << ".balignl"; break;
  case 8: llvm_unreachable("Unsupported alignment size!");
  }

  OS << ' ' << ByteAlignment;
  OS << ", " << truncateToSize(Value, ValueSize);
  if (MaxBytesToEmit)
    OS << ", " << MaxBytesToEmit;
  EmitEOL();
}

void PTXMCAsmStreamer::EmitCodeAlignment(unsigned ByteAlignment,
                                         unsigned MaxBytesToEmit) {}

void PTXMCAsmStreamer::EmitValueToOffset(const MCExpr *Offset,
                                         unsigned char Value) {}


void PTXMCAsmStreamer::EmitFileDirective(StringRef Filename) {
  assert(MAI.hasSingleParameterDotFile());
  OS << "\t.file\t";
  PrintQuotedString(Filename, OS);
  EmitEOL();
}

void PTXMCAsmStreamer::EmitDwarfFileDirective(unsigned FileNo,
                                              StringRef Filename){
  OS << "\t.file\t" << FileNo << ' ';
  PrintQuotedString(Filename, OS);
  EmitEOL();
}

void PTXMCAsmStreamer::AddEncodingComment(const MCInst &Inst) {}

void PTXMCAsmStreamer::EmitInstruction(const MCInst &Inst) {
  assert(CurSection && "Cannot emit contents before setting section!");

  // Show the encoding in a comment if we have a code emitter.
  if (Emitter)
    AddEncodingComment(Inst);

  // Show the MCInst if enabled.
  if (ShowInst) {
    Inst.dump_pretty(GetCommentOS(), &MAI, InstPrinter.get(), "\n ");
    GetCommentOS() << "\n";
  }

  // If we have an AsmPrinter, use that to print, otherwise print the MCInst.
  if (InstPrinter)
    InstPrinter->printInst(&Inst, OS);
  else
    Inst.print(OS, &MAI);
  EmitEOL();
}

/// EmitRawText - If this file is backed by an assembly streamer, this dumps
/// the specified string in the output .s file.  This capability is
/// indicated by the hasRawTextSupport() predicate.
void PTXMCAsmStreamer::EmitRawText(StringRef String) {
  if (!String.empty() && String.back() == '\n')
    String = String.substr(0, String.size()-1);
  OS << String;
  EmitEOL();
}

void PTXMCAsmStreamer::Finish() {}

MCStreamer *llvm::createPTXAsmStreamer(MCContext &Context,
                                       formatted_raw_ostream &OS,
                                       bool isLittleEndian,
                                       bool isVerboseAsm, MCInstPrinter *IP,
                                       MCCodeEmitter *CE, bool ShowInst) {
  return new PTXMCAsmStreamer(Context, OS, isLittleEndian, isVerboseAsm,
                              IP, CE, ShowInst);
}