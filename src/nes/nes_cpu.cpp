#include "nes_cpu.hpp"

#include <array>
#include <functional>
#include <string_view>

#include <fmt/format.h>

#include "nes.hpp"
#include "nes_cpu_ops.hpp"

#include <util/logging.hpp>

namespace nesem
{

	struct Op
	{
		using Fn = bool (nesem::NesCpu::*)();
		std::string_view name;
		Fn op;
	};

#define OP(ins)                    \
	Op                             \
	{                              \
		.name = #ins,              \
		.op = &nesem::NesCpu::ins, \
	}

	struct CpuOps
	{
		//      0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
		//   ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
		// 0 │ BRK │ ORA │  -  │  -  │  -  │ ORA │ ASL │  -  │ PHP │ ORA │ ASL │  -  │  -  │ ORA │ ASL │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// 1 │ BPL │ ORA │  -  │  -  │  -  │ ORA │ ASL │  -  │ CLC │ ORA │  -  │  -  │  -  │ ORA │ ASL │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// 2 │ JSR │ AND │  -  │  -  │ BIT │ AND │ ROL │  -  │ PLP │ AND │ ROL │  -  │ BIT │ AND │ ROL │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// 3 │ BMI │ AND │  -  │  -  │  -  │ AND │ ROL │  -  │ SEC │ AND │  -  │  -  │  -  │ AND │ ROL │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// 4 │ RTI │ EOR │  -  │  -  │  -  │ EOR │ LSR │  -  │ PHA │ EOR │ LSR │  -  │ JMP │ EOR │ LSR │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// 5 │ BVC │ EOR │  -  │  -  │  -  │ EOR │ LSR │  -  │ CLI │ EOR │  -  │  -  │  -  │ EOR │ LSR │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// 6 │ RTS │ ADC │  -  │  -  │  -  │ ADC │ ROR │  -  │ PLA │ ADC │ ROR │  -  │ JMP │ ADC │ ROR │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// 7 │ BVS │ ADC │  -  │  -  │  -  │ ADC │ ROR │  -  │ SEI │ ADC │  -  │  -  │  -  │ ADC │ ROR │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// 8 │  -  │ STA │  -  │  -  │ STY │ STA │ STX │  -  │ DEY │  -  │ TXA │  -  │ STY │ STA │ STX │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// 9 │ BCC │ STA │  -  │  -  │ STY │ STA │ STX │  -  │ TYA │ STA │ TXS │  -  │  -  │ STA │  -  │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// a │ LDY │ LDA │ LDX │  -  │ LDY │ LDA │ LDX │  -  │ TAY │ LDA │ TAX │  -  │ LDY │ LDA │ LDX │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// b │ BCS │ LDA │  -  │  -  │ LDY │ LDA │ LDX │  -  │ CLV │ LDA │ TSX │  -  │ LDY │ LDA │ LDX │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// c │ CPY │ CMP │  -  │  -  │ CPY │ CMP │ DEC │  -  │ INY │ CMP │ DEX │  -  │ CPY │ CMP │ DEC │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// d │ BNE │ CMP │  -  │  -  │  -  │ CMP │ DEC │  -  │ CLD │ CMP │  -  │  -  │  -  │ CMP │ DEC │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// e │ CPX │ SBC │  -  │  -  │ CPX │ SBC │ INC │  -  │ INX │ SBC │ NOP │  -  │ CPX │ SBC │ INC │  -  │
		//   ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
		// f │ BEQ │ SBC │  -  │  -  │  -  │ SBC │ INC │  -  │ SED │ SBC │  -  │  -  │  -  │ SBC │ INC │  -  │
		//   └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
		// total ops: 56

		constexpr static std::array ops = {
			// clang-format off
			// 0        1        2        3        4        5        6        7        8        9        A        B        C        D        E        F
			OP(BRK), OP(ORA), OP(xxx), OP(xxx), OP(xxx), OP(ORA), OP(ASL), OP(xxx), OP(PHP), OP(ORA), OP(ASL), OP(xxx), OP(xxx), OP(ORA), OP(ASL), OP(xxx), // 0
			OP(BPL), OP(ORA), OP(xxx), OP(xxx), OP(xxx), OP(ORA), OP(ASL), OP(xxx), OP(CLC), OP(ORA), OP(xxx), OP(xxx), OP(xxx), OP(ORA), OP(ASL), OP(xxx), // 1
			OP(JSR), OP(AND), OP(xxx), OP(xxx), OP(BIT), OP(AND), OP(ROL), OP(xxx), OP(PLP), OP(AND), OP(ROL), OP(xxx), OP(BIT), OP(AND), OP(ROL), OP(xxx), // 2
			OP(BMI), OP(AND), OP(xxx), OP(xxx), OP(xxx), OP(AND), OP(ROL), OP(xxx), OP(SEC), OP(AND), OP(xxx), OP(xxx), OP(xxx), OP(AND), OP(ROL), OP(xxx), // 3
			OP(RTI), OP(EOR), OP(xxx), OP(xxx), OP(xxx), OP(EOR), OP(LSR), OP(xxx), OP(PHA), OP(EOR), OP(LSR), OP(xxx), OP(JMP), OP(EOR), OP(LSR), OP(xxx), // 4
			OP(BVC), OP(EOR), OP(xxx), OP(xxx), OP(xxx), OP(EOR), OP(LSR), OP(xxx), OP(CLI), OP(EOR), OP(xxx), OP(xxx), OP(xxx), OP(EOR), OP(LSR), OP(xxx), // 5
			OP(RTS), OP(ADC), OP(xxx), OP(xxx), OP(xxx), OP(ADC), OP(ROR), OP(xxx), OP(PLA), OP(ADC), OP(ROR), OP(xxx), OP(JMP), OP(ADC), OP(ROR), OP(xxx), // 6
			OP(BVS), OP(ADC), OP(xxx), OP(xxx), OP(xxx), OP(ADC), OP(ROR), OP(xxx), OP(SEI), OP(ADC), OP(xxx), OP(xxx), OP(xxx), OP(ADC), OP(ROR), OP(xxx), // 7
			OP(xxx), OP(STA), OP(xxx), OP(xxx), OP(STY), OP(STA), OP(STX), OP(xxx), OP(DEY), OP(xxx), OP(TXA), OP(xxx), OP(STY), OP(STA), OP(STX), OP(xxx), // 8
			OP(BCC), OP(STA), OP(xxx), OP(xxx), OP(STY), OP(STA), OP(STX), OP(xxx), OP(TYA), OP(STA), OP(TXS), OP(xxx), OP(xxx), OP(STA), OP(xxx), OP(xxx), // 9
			OP(LDY), OP(LDA), OP(LDX), OP(xxx), OP(LDY), OP(LDA), OP(LDX), OP(xxx), OP(TAY), OP(LDA), OP(TAX), OP(xxx), OP(LDY), OP(LDA), OP(LDX), OP(xxx), // A
			OP(BCS), OP(LDA), OP(xxx), OP(xxx), OP(LDY), OP(LDA), OP(LDX), OP(xxx), OP(CLV), OP(LDA), OP(TSX), OP(xxx), OP(LDY), OP(LDA), OP(LDX), OP(xxx), // B
			OP(CPY), OP(CMP), OP(xxx), OP(xxx), OP(CPY), OP(CMP), OP(DEC), OP(xxx), OP(INY), OP(CMP), OP(DEX), OP(xxx), OP(CPY), OP(CMP), OP(DEC), OP(xxx), // C
			OP(BNE), OP(CMP), OP(xxx), OP(xxx), OP(xxx), OP(CMP), OP(DEC), OP(xxx), OP(CLD), OP(CMP), OP(xxx), OP(xxx), OP(xxx), OP(CMP), OP(DEC), OP(xxx), // D
			OP(CPX), OP(SBC), OP(xxx), OP(xxx), OP(CPX), OP(SBC), OP(INC), OP(xxx), OP(INX), OP(SBC), OP(NOP), OP(xxx), OP(CPX), OP(SBC), OP(INC), OP(xxx), // E
			OP(BEQ), OP(SBC), OP(xxx), OP(xxx), OP(xxx), OP(SBC), OP(INC), OP(xxx), OP(SED), OP(SBC), OP(xxx), OP(xxx), OP(xxx), OP(SBC), OP(INC), OP(xxx), // F
			// clang-format on
		};

		static_assert(size(ops) == 256);
	};

#undef OP

	// Pseudo instructions used to distinguish other events
	constexpr int startup_sequence = -1;
	constexpr int nmi_sequence = -2;
	constexpr int irq_sequence = -3;

	std::string decompile(U8 instruction, NesBus &bus, U16 pc) noexcept
	{
		std::string_view name = CpuOps::ops[instruction].name;

		switch (instruction)
		{
		default:
			return fmt::format("Unknown: {:02X}", instruction);

		// Implied 0 - "INS"
		case 0x00:
		case 0x18:
		case 0xD8:
		case 0x58:
		case 0xB8:
		case 0xCA:
		case 0x88:
		case 0xE8:
		case 0xC8:
		case 0xEA:
		case 0x48:
		case 0x08:
		case 0x68:
		case 0x28:
		case 0x40:
		case 0x60:
		case 0x38:
		case 0xF8:
		case 0x78:
		case 0xAA:
		case 0xA8:
		case 0xBA:
		case 0x8A:
		case 0x9A:
		case 0x98:
			return std::string(name);

		// Accumulator 0 - "INS A"
		case 0x0A:
		case 0x4A:
		case 0x2A:
		case 0x6A:
			return fmt::format("{} A", name);

		// Relative 1 - "INS *+/-num <$addr>" (normally, you jump to a label, but we'll calculate the absolute address)
		case 0x90:
		case 0xB0:
		case 0xF0:
		case 0x30:
		case 0xD0:
		case 0x10:
		case 0x50:
		case 0x70:
		{
			// relative address is signed
			int value = int8_t(bus.read(pc));
			return fmt::format("{} *{:+} <${:04X}>", name, value, pc + value + 1);
		}

		// Immediate 1 - "INS #dec <$hex>"
		case 0x69:
		case 0x29:
		case 0xC9:
		case 0xE0:
		case 0xC0:
		case 0x49:
		case 0xA9:
		case 0xA2:
		case 0xA0:
		case 0x09:
		case 0xE9:
		{
			int value = bus.read(pc);
			return fmt::format("{0} #{1} <${1:02X}>", name, value);
		}

		// Zero Page 1 - "INS $hex"
		case 0x65:
		case 0x25:
		case 0x06:
		case 0x24:
		case 0xC5:
		case 0xE4:
		case 0xC4:
		case 0xC6:
		case 0x45:
		case 0xE6:
		case 0xA5:
		case 0xA6:
		case 0xA4:
		case 0x46:
		case 0x05:
		case 0x26:
		case 0x66:
		case 0xE5:
		case 0x85:
		case 0x86:
		case 0x84:
		{
			int value = bus.read(pc);
			return fmt::format("{} ${:02X}", name, value);
		}

		// Zero Page,X 1 - "INS $hex,X"
		case 0x75:
		case 0x35:
		case 0x16:
		case 0xD5:
		case 0xD6:
		case 0x55:
		case 0xF6:
		case 0xB5:
		case 0xB4:
		case 0x56:
		case 0x15:
		case 0x36:
		case 0x76:
		case 0xF5:
		case 0x95:
		case 0x94:
		{
			int value = bus.read(pc);
			return fmt::format("{} ${:02X},X", name, value);
		}

		// Zero Page,Y 1 - "INS $hex,Y"
		case 0xB6:
		case 0x96:
		{
			int value = bus.read(pc);
			return fmt::format("{} ${:02X},Y", name, value);
		}

		// Absolute 2 - "INS $addr"
		case 0x6D:
		case 0x2D:
		case 0x0E:
		case 0x2C:
		case 0xCD:
		case 0xEC:
		case 0xCC:
		case 0xCE:
		case 0x4D:
		case 0xEE:
		case 0x4C:
		case 0x20:
		case 0xAD:
		case 0xAE:
		case 0xAC:
		case 0x4E:
		case 0x0D:
		case 0x2E:
		case 0x6E:
		case 0xED:
		case 0x8D:
		case 0x8E:
		case 0x8C:
		{
			int lo = bus.read(pc);
			int hi = bus.read(pc + 1);
			return fmt::format("{} ${:04X}", name, hi << 8 | lo);
		}

		// Absolute,X 2 - "INS $addr,X"
		case 0x7D:
		case 0x3D:
		case 0x1E:
		case 0xDD:
		case 0xDE:
		case 0x5D:
		case 0xFE:
		case 0xBD:
		case 0xBC:
		case 0x5E:
		case 0x1D:
		case 0x3E:
		case 0x7E:
		case 0xFD:
		case 0x9D:
		{
			int lo = bus.read(pc);
			int hi = bus.read(pc + 1);
			return fmt::format("{} ${:04X},X", name, hi << 8 | lo);
		}

		// Absolute,Y 2 - "INS $addr,Y"
		case 0x79:
		case 0x39:
		case 0xD9:
		case 0x59:
		case 0xB9:
		case 0xBE:
		case 0x19:
		case 0xF9:
		case 0x99:
		{
			int lo = bus.read(pc);
			int hi = bus.read(pc + 1);
			return fmt::format("{} ${:04X},Y", name, hi << 8 | lo);
		}

		// Indirect 2 - "INS ($addr)"
		case 0x6C:
		{
			int lo = bus.read(pc);
			int hi = bus.read(pc + 1);
			return fmt::format("{} (${:04X})", name, hi << 8 | lo);
		}

		// (Indirect,X) 1 - "INS ($zp,X)"
		case 0x61:
		case 0x21:
		case 0xC1:
		case 0x41:
		case 0xA1:
		case 0x01:
		case 0xE1:
		case 0x81:
		{
			int value = bus.read(pc);
			return fmt::format("{} (${:02X},X)", name, value);
		}

		// (Indirect),Y 1 - INS ($zp),Y
		case 0x71:
		case 0x31:
		case 0xD1:
		case 0x51:
		case 0xB1:
		case 0x11:
		case 0xF1:
		case 0x91:
		{
			int value = bus.read(pc);
			return fmt::format("{} (${:02X}),Y", name, value);
		}
		}
	}

	// format the instruction in an nestest.log compatible format
	std::string format_nestest(U8 instruction, NesBus &bus, U16 pc) noexcept
	{
		std::string_view name = CpuOps::ops[instruction].name;

		//                      PC     INS   DISASM
		constexpr auto fmt = "{0:04X}  {1:9} {2:31}";

		switch (instruction)
		{
		default:
			return fmt::format("Unknown: {:02X}", instruction);

		// Implied 0 - "INS"
		case 0x00:
		case 0x18:
		case 0xD8:
		case 0x58:
		case 0xB8:
		case 0xCA:
		case 0x88:
		case 0xE8:
		case 0xC8:
		case 0xEA:
		case 0x48:
		case 0x08:
		case 0x68:
		case 0x28:
		case 0x40:
		case 0x60:
		case 0x38:
		case 0xF8:
		case 0x78:
		case 0xAA:
		case 0xA8:
		case 0xBA:
		case 0x8A:
		case 0x9A:
		case 0x98:
			return fmt::format(fmt, pc - 1, fmt::format("{:02X}", instruction), name);

		// Accumulator 0 - "INS A"
		case 0x0A:
		case 0x4A:
		case 0x2A:
		case 0x6A:
			return fmt::format(fmt, pc - 1, fmt::format("{:02X}", instruction), fmt::format("{} A", name));

		// Relative 1 - "INS *+/-num <$addr>" (normally, you jump to a label, but we'll calculate the absolute address)
		case 0x90:
		case 0xB0:
		case 0xF0:
		case 0x30:
		case 0xD0:
		case 0x10:
		case 0x50:
		case 0x70:
		{
			// relative address is signed
			int value = int8_t(bus.read(pc));
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X}", instruction, value), fmt::format("{} ${:04X}", name, pc + value + 1));
		}

		// Immediate 1 - "INS #dec <$hex>"
		case 0x69:
		case 0x29:
		case 0xC9:
		case 0xE0:
		case 0xC0:
		case 0x49:
		case 0xA9:
		case 0xA2:
		case 0xA0:
		case 0x09:
		case 0xE9:
		{
			int value = bus.read(pc);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X}", instruction, value), fmt::format("{0} #${1:02X}", name, value));
		}

		// Zero Page 1 - "INS $hex"
		case 0x65:
		case 0x25:
		case 0x06:
		case 0x24:
		case 0xC5:
		case 0xE4:
		case 0xC4:
		case 0xC6:
		case 0x45:
		case 0xE6:
		case 0xA5:
		case 0xA6:
		case 0xA4:
		case 0x46:
		case 0x05:
		case 0x26:
		case 0x66:
		case 0xE5:
		case 0x85:
		case 0x86:
		case 0x84:
		{
			int value = bus.read(pc);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X}", instruction, value), fmt::format("{} ${:02X}", name, value));
		}

		// Zero Page,X 1 - "INS $hex,X"
		case 0x75:
		case 0x35:
		case 0x16:
		case 0xD5:
		case 0xD6:
		case 0x55:
		case 0xF6:
		case 0xB5:
		case 0xB4:
		case 0x56:
		case 0x15:
		case 0x36:
		case 0x76:
		case 0xF5:
		case 0x95:
		case 0x94:
		{
			int value = bus.read(pc);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X}", instruction, value), fmt::format("{} ${:02X},X", name, value));
		}

		// Zero Page,Y 1 - "INS $hex,Y"
		case 0xB6:
		case 0x96:
		{
			int value = bus.read(pc);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X}", instruction, value), fmt::format("{} ${:02X},Y", name, value));
		}

		// Absolute 2 - "INS $addr"
		case 0x6D:
		case 0x2D:
		case 0x0E:
		case 0x2C:
		case 0xCD:
		case 0xEC:
		case 0xCC:
		case 0xCE:
		case 0x4D:
		case 0xEE:
		case 0x4C:
		case 0x20:
		case 0xAD:
		case 0xAE:
		case 0xAC:
		case 0x4E:
		case 0x0D:
		case 0x2E:
		case 0x6E:
		case 0xED:
		case 0x8D:
		case 0x8E:
		case 0x8C:
		{
			int lo = bus.read(pc);
			int hi = bus.read(pc + 1);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X} {:02X}", instruction, lo, hi), fmt::format("{} ${:04X}", name, hi << 8 | lo));
		}

		// Absolute,X 2 - "INS $addr,X"
		case 0x7D:
		case 0x3D:
		case 0x1E:
		case 0xDD:
		case 0xDE:
		case 0x5D:
		case 0xFE:
		case 0xBD:
		case 0xBC:
		case 0x5E:
		case 0x1D:
		case 0x3E:
		case 0x7E:
		case 0xFD:
		case 0x9D:
		{
			int lo = bus.read(pc);
			int hi = bus.read(pc + 1);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X} {:02X}", instruction, lo, hi), fmt::format("{} ${:04X},X", name, hi << 8 | lo));
		}

		// Absolute,Y 2 - "INS $addr,Y"
		case 0x79:
		case 0x39:
		case 0xD9:
		case 0x59:
		case 0xB9:
		case 0xBE:
		case 0x19:
		case 0xF9:
		case 0x99:
		{
			int lo = bus.read(pc);
			int hi = bus.read(pc + 1);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X} {:02X}", instruction, lo, hi), fmt::format("{} ${:04X},Y", name, hi << 8 | lo));
		}

		// Indirect 2 - "INS ($addr)"
		case 0x6C:
		{
			int lo = bus.read(pc);
			int hi = bus.read(pc + 1);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X} {:02X}", instruction, lo, hi), fmt::format("{} (${:04X})", name, hi << 8 | lo));
		}

		// (Indirect,X) 1 - "INS ($zp,X)"
		case 0x61:
		case 0x21:
		case 0xC1:
		case 0x41:
		case 0xA1:
		case 0x01:
		case 0xE1:
		case 0x81:
		{
			int value = bus.read(pc);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X}", instruction, value), fmt::format("{} (${:02X},X)", name, value));
		}

		// (Indirect),Y 1 - INS ($zp),Y
		case 0x71:
		case 0x31:
		case 0xD1:
		case 0x51:
		case 0xB1:
		case 0x11:
		case 0xF1:
		case 0x91:
		{
			int value = bus.read(pc);
			return fmt::format(fmt, pc - 1, fmt::format("{:02X} {:02X}", instruction, value), fmt::format("{} (${:02X}),Y", name, value));
		}
		}
	}

	// TODO: Keep this? Give it a "better" home? The static logger is a bit hacky...
	void NesCpu::log_instruction() noexcept
	{
		if constexpr (SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG)
		{
			static std::shared_ptr<spdlog::logger> cpu_log = [] {
				auto log_filename = std::filesystem::temp_directory_path() / "cpu.log";

				LOG_INFO("CPU log created at: {}", log_filename.string());
				auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_filename.string(), true);

				// no extra detail, just the log message
				file_sink->set_pattern("%v");

				return std::make_shared<spdlog::logger>("CPU log", file_sink);
			}();

			// designed to be compatible with nestest.log
			// pc  raw bytes *di
			cpu_log->debug("{} A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X} PPU: XXX,XXX CYC:{}", format_nestest(U8(instruction), nes->bus(), PC), A, X, Y, U8(P), S, cycles);
		}
	}

	NesCpu::NesCpu(Nes *nes) noexcept
		: nes(nes)
	{
		CHECK(nes != nullptr, "nes is required");
		reset();
	}

	// signals
	void NesCpu::reset(U16 addr) noexcept
	{
		PC = addr;

		// for some reason, the stack pointer is decremented on start and on reset
		// This is the "correct" value after initial power-on
		S = 0xFD;
		P = ProcessorStatus::Default;

		nmi_requested = false;
		step = 0;
		cycles = 0;

		if (PC != 0)
		{
			// if we set a start address, just start executing and fast-forward the cycles for the startup
			instruction = 0;
			cycles = 6;
		}
		else
			// otherwise, emulate the startup sequence
			instruction = startup_sequence;
	}

	bool NesCpu::interrupt_requested() noexcept
	{
		using enum ProcessorStatus;

		return ((P & I) == None) && nes->interrupt_requested();
	}

	void NesCpu::nmi() noexcept
	{
		// non-maskable interrupts cannot be ignored
		nmi_requested = true;
	}

	void NesCpu::dma(U8 page) noexcept
	{
		in_dma = true;
		dma_step = -1;
		dma_page = page;
	}

	bool NesCpu::clock() noexcept
	{
		bool instruction_complete = false;
		++cycles;

		// dma bypasses normal cpu operation
		if (in_dma)
		{
			CHECK(step == 0, "The write that triggered the DMA should have been the last step");

			// dma has a one tick wait cycle to let the write finish
			if (dma_step < 0)
			{
				++dma_step;
				return instruction_complete;
			}

			// dma starts on an even cycle for some reason. skip this tick if we are ready but on an odd cycle
			if (dma_step == 0 && (cycles & 1) == 1)
				return instruction_complete;

			// do the dma transfer, read on even steps, write on odd steps
			if ((dma_step & 1) == 0)
				scratch = nes->bus().read((dma_page << 8) | (dma_step >> 1));
			else
				nes->ppu().oamdata(scratch);

			++dma_step;

			// done once we've read and written the full 256 bytes for the page (256 reads + 256 writes == 512 steps)
			if (dma_step >= 512)
			{
				in_dma = false;
				instruction_complete = true;
			}

			return instruction_complete;
		}

		++step;

		// check for reset sequence
		// reset takes 7 cycles
		if (instruction == startup_sequence)
		{
			if (step == 6)
				PC = nes->bus().read(0xFFFC);
			else if (step == 7)
			{
				PC |= nes->bus().read(0xFFFD) << 8;

				// reset finished. set instruction to a dummy value (BRK in this case)
				instruction = 0;
				step = 0;
				instruction_complete = true;
			}

			return instruction_complete;
		}

		// TODO: NMI should be able to interrupt an irq... I think
		else if (instruction == nmi_sequence || instruction == irq_sequence)
		{
			using enum ProcessorStatus;
			switch (step)
			{
			case 3:
				push(U8((PC >> 8) & 0xFF));
				break;
			case 4:
				push(U8(PC & 0xFF));
				break;
			case 5:
				P |= I;
				push(U8(P));
				break;
			case 6:
				if (instruction == nmi_sequence)
					PC = nes->bus().read(0xFFFA);
				else if (instruction == irq_sequence)
					PC = nes->bus().read(0xFFFE);

				break;
			case 7:
				if (instruction == nmi_sequence)
					PC |= nes->bus().read(0xFFFB) << 8;
				else if (instruction == irq_sequence)
					PC |= nes->bus().read(0xFFFF) << 8;

				instruction = 0;
				step = 0;
				instruction_complete = true;

				break;
			}
			return instruction_complete;
		}

		if (step == 1)
		{
			if (nmi_requested)
			{
				nmi_requested = false;
				instruction = nmi_sequence;
				return instruction_complete;
			}
			else if (interrupt_requested())
			{
				instruction = irq_sequence;
				return instruction_complete;
			}

			instruction = readPC();
			LOG_DEBUG("{:>5}: [{:04X}] {:<35} A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X}", cycles, PC - 1, decompile(U8(instruction), nes->bus(), PC), A, X, Y, U8(P), S);
			log_instruction();
		}
		else
		{
			// instructions return true when completed, so reset the step counter
			if (std::invoke(CpuOps::ops[instruction].op, this))
			{
				step = 0;
				instruction_complete = true;
			}
		}

		return instruction_complete;
	}

	void NesCpu::push(U8 value) noexcept
	{
		nes->bus().write(0x0100 | S, value);
		--S;
	}

	U8 NesCpu::pop() noexcept
	{
		++S;
		return nes->bus().read(0x0100 | S);
	}

	U8 NesCpu::readPC() noexcept
	{
		return nes->bus().read(PC++);
	}

	bool NesCpu::branch(bool condition) noexcept
	{
		// Relative addressing mode, variable cycles
		// 2 if no branch
		// 3 if local page branch
		// 4 if page boundary crossed
		switch (step)
		{
		case 2:
			// make sure we read the next value, even if the condition ends up failing
			scratch = readPC();

			// if condition failed, we are done
			if (!condition)
				return true;

			break;

		case 3:
		{
			// condition true, so increment PC by the relative offset we stored in scratch
			// Note: record the hi byte, if we overflow, we need an extra cycle to "correct" PC
			auto hi = PC & 0xFF00;

			// address is 8-bit signed variable, so cast needed
			PC += int8_t(scratch);

			// we didn't cross a page boundary, so we are done
			if ((PC & 0xFF00) == hi)
				return true;

			break;
		}

		case 4:
			// extra cycle due to crossing page boundary, done
			return true;
		}

		return false;
	}

	// invalid instruction
	bool NesCpu::xxx() noexcept
	{
		LOG_CRITICAL("{:>5}: PC:{:04X} A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X}", cycles, PC - 1, A, X, Y, U8(P), S);
		LOG_CRITICAL("Invalid instruction {:02X}", instruction);

		nes->error("Invalid CPU instruction");
		return true;
	}

	NesCpu::AddressStatus NesCpu::read() noexcept
	{
		switch ((instruction & 0b00011100) >> 2)
		{
		case 0:
			switch (instruction)
			{
			// exceptions, these 4 are immediate mode
			case 0xA0: // LDY
			case 0xA2: // LDX
			case 0xE0: // CPX
			case 0xC0: // CPY
				return IMM(OpType::read);

			default:
				// every other read is Indirect X
				return INX(OpType::read);
			}

		case 1:
			return ZP(OpType::read);

		case 2:
			return IMM(OpType::read);

		case 3:
			return ABS(OpType::read);

		case 4:
			return INY(OpType::read);

		case 5:
			// LDX indexes ZP on Y, probably because we are trying to overwrite X
			if (instruction == 0xB6)
				return ZPY(OpType::read);

			return ZPX(OpType::read);

		case 6:
			return ABY(OpType::read);

		case 7:
			// LDX uses Y for the offset, probably because we are trying to overwrite X
			if (instruction == 0xBE)
				return ABY(OpType::read);

			return ABX(OpType::read);
		}

		// Annoying that compilers would even warn about the above switch, it
		// switches on a 3 bit value that can only be 0-7, and each case is handled...
		CHECK(false, "We should never reach here");
		return AddressStatus::complete;
	}

	NesCpu::AddressStatus NesCpu::write() noexcept
	{
		switch ((instruction & 0b00011100) >> 2)
		{
		case 0:
			return INX(OpType::write);

		case 1:
			return ZP(OpType::write);

		case 2:
			CHECK(false, "Unused write address mode, we shouldn't be here");
			break;

		case 3:
			return ABS(OpType::write);

		case 4:
			return INY(OpType::write);

		case 5:
			// STX uses Y for offset, probably because it is modifying X
			if (instruction == 0x96)
				return ZPY(OpType::write);

			return ZPX(OpType::write);

		case 6:
			return ABY(OpType::write);

		case 7:
			return ABX(OpType::write);
		}

		CHECK(false, "We should never reach here");
		return AddressStatus::complete;
	}

	NesCpu::AddressStatus NesCpu::read_modify_write() noexcept
	{
		switch ((instruction & 0b00011100) >> 2)
		{
		case 1:
			return ZP(OpType::read_modify_write);
		case 2:
			return AddressStatus::accumulator;
		case 3:
			return ABS(OpType::read_modify_write);
		case 5:
			return ZPX(OpType::read_modify_write);
		case 7:
			return ABX(OpType::read_modify_write);

		case 0:
		case 4:
		case 6:
			CHECK(false, "Unused read-modify-write address mode, we shouldn't be here");
			break;
		}

		CHECK(false, "We should never reach here");
		return AddressStatus::complete;
	}

	// Add with carry
	bool NesCpu::ADC() noexcept
	{
		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			auto result = op::ADC(A, scratch, P);
			A = result.ans;
			P = result.flags;
			return true;
		}

		return false;
	}

	// Logical AND
	bool NesCpu::AND() noexcept
	{
		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			auto result = op::AND(A, scratch, P);
			A = result.ans;
			P = result.flags;
			return true;
		}

		return false;
	}

	// Arithmetic Shift Left
	bool NesCpu::ASL() noexcept
	{
		auto status = read_modify_write();

		// Modifying the accumulator can be completed immediately
		if (status == AddressStatus::accumulator)
		{
			auto result = op::ASL(A, P);
			A = result.ans;
			P = result.flags;
			return true;
		}

		if (status == AddressStatus::complete)
			return true;

		// we can write our answer to scratch anytime after read_complete up to write_ready,
		// the value will be preserved
		// We'll wait until the last moment to write, but it shouldn't matter
		if (status == AddressStatus::write_ready)
		{
			auto result = op::ASL(scratch, P);
			scratch = result.ans;
			P = result.flags;
		}

		return false;
	}

	// Branch if Carry Clear
	bool NesCpu::BCC() noexcept
	{
		using enum ProcessorStatus;
		return branch((P & C) == None);
	}

	// Branch if Carry Set
	bool NesCpu::BCS() noexcept
	{
		using enum ProcessorStatus;
		return branch((P & C) == C);
	}

	// Branch if Equal
	bool NesCpu::BEQ() noexcept
	{
		using enum ProcessorStatus;
		return branch((P & Z) == Z);
	}

	// Bit Test
	bool NesCpu::BIT() noexcept
	{
		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			P = op::BIT(A, scratch, P);
			return true;
		}

		return false;
	}

	// Branch if Minus
	bool NesCpu::BMI() noexcept
	{
		using enum ProcessorStatus;
		return branch((P & N) == N);
	}

	// Branch if Not Equal
	bool NesCpu::BNE() noexcept
	{
		using enum ProcessorStatus;
		return branch((P & Z) == None);
	}

	// Branch if Positive
	bool NesCpu::BPL() noexcept
	{
		using enum ProcessorStatus;
		return branch((P & N) == None);
	}

	// Force Interrupt
	bool NesCpu::BRK() noexcept
	{
		// Implied addressing mode
		using enum ProcessorStatus;

		switch (step)
		{
		case 2:
			// BRK does a dummy read on the next value, making this a
			// two-byte instruction, but it doesn't do anything with it
			++PC;
			break;

		case 3:
			// push PCH on stack
			P |= B;
			push(U8(PC >> 8));
			break;

		case 4:
			// push PCL on stack
			push(U8(PC));
			break;

		case 5:
			// push P on stack
			// We set B as it technically isn't part of the processor status, but indicates whether
			// hardware interrupts (nmi or irq) or software (BRK, PHP) pushed the status to the stack
			push(U8(P | B));
			break;

		case 6:
			// fetch PCL from $FFFE
			PC = nes->bus().read(0xFFFE);
			break;

		case 7:
			// fetch PCH from $FFFF
			PC = (nes->bus().read(0xFFFF) << 8) | PC;
			return true;
		}
		return false;
	}

	// Branch if Overflow Clear
	bool NesCpu::BVC() noexcept
	{
		using enum ProcessorStatus;
		return branch((P & V) == None);
	}

	// Branch if Overflow Set
	bool NesCpu::BVS() noexcept
	{
		using enum ProcessorStatus;
		return branch((P & V) == V);
	}

	// Clear Carry Flag
	bool NesCpu::CLC() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		P &= ~C;
		return true;
	}

	// Clear Decimal Mode
	bool NesCpu::CLD() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		P &= ~D;
		return true;
	}

	// Clear Interrupt Disable
	bool NesCpu::CLI() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		P &= ~I;
		return true;
	}

	// Clear Overflow Flag
	bool NesCpu::CLV() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		P &= ~V;
		return true;
	}

	// Compare
	bool NesCpu::CMP() noexcept
	{
		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			P = op::CMP(A, scratch, P);
			return true;
		}

		return false;
	}

	// Compare X Register
	bool NesCpu::CPX() noexcept
	{
		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			P = op::CMP(X, scratch, P);
			return true;
		}

		return false;
	}

	// Compare Y Register
	bool NesCpu::CPY() noexcept
	{
		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			P = op::CMP(Y, scratch, P);
			return true;
		}

		return false;
	}

	// Decrement Memory
	bool NesCpu::DEC() noexcept
	{
		using enum ProcessorStatus;

		auto status = read_modify_write();

		if (status == AddressStatus::complete)
			return true;

		// we can write our answer to scratch anytime after read_complete up to write_ready,
		// the value will be preserved
		// We'll wait until the last moment to write, but it shouldn't matter
		if (status == AddressStatus::write_ready)
		{
			--scratch;

			if (scratch == 0)
				P |= Z;
			else
				P &= ~Z;

			if (scratch & 0b1000'0000)
				P |= N;
			else
				P &= ~N;
		}

		return false;
	}

	// Decrement X Register
	bool NesCpu::DEX() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		--X;

		if (X == 0)
			P |= Z;
		else
			P &= ~Z;

		if (X & 0b1000'0000)
			P |= N;
		else
			P &= ~N;

		return true;
	}

	// Decrement Y Register
	bool NesCpu::DEY() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		--Y;

		if (Y == 0)
			P |= Z;
		else
			P &= ~Z;

		if (Y & 0b1000'0000)
			P |= N;
		else
			P &= ~N;

		return true;
	}

	// Exclusive OR
	bool NesCpu::EOR() noexcept
	{
		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			auto result = op::EOR(A, scratch, P);
			A = result.ans;
			P = result.flags;
			return true;
		}

		return false;
	}

	// Increment Memory
	bool NesCpu::INC() noexcept
	{
		using enum ProcessorStatus;

		auto status = read_modify_write();

		if (status == AddressStatus::complete)
			return true;

		// we can write our answer to scratch anytime after read_complete up to write_ready,
		// the value will be preserved
		// We'll wait until the last moment to write, but it shouldn't matter
		if (status == AddressStatus::write_ready)
		{
			++scratch;

			if (scratch == 0)
				P |= Z;
			else
				P &= ~Z;

			if (scratch & 0b1000'0000)
				P |= N;
			else
				P &= ~N;
		}

		return false;
	}

	// Increment X Register
	bool NesCpu::INX() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		++X;

		if (X == 0)
			P |= Z;
		else
			P &= ~Z;

		if (X & 0b1000'0000)
			P |= N;
		else
			P &= ~N;

		return true;
	}

	// Increment Y Register
	bool NesCpu::INY() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		++Y;

		if (Y == 0)
			P |= Z;
		else
			P &= ~Z;

		if (Y & 0b1000'0000)
			P |= N;
		else
			P &= ~N;

		return true;
	}

	// Jump
	bool NesCpu::JMP() noexcept
	{
		switch (step)
		{
		case 2:
			scratch = readPC();
			break;

		case 3:
		{
			PC = (readPC() << 8) | scratch;

			// if this is an absolute addressing mode JMP, we are done
			if (instruction == 0x4C)
				return true;
			break;
		}

		// Absolute indirect addressing beyond this point, load the address
		// stored at the location pointed to by the immediate data
		// NOTE: if PC overflows, it does *not* cross the page boundary
		case 4:
		{
			// record our current page
			auto hi = PC & 0xFF00;

			scratch = readPC();

			// force PC to current page, e.g. 0x02FF + 1 == 0x0200
			PC = hi | (PC & 0x00FF);
			break;
		}

		case 5:
			PC = (readPC() << 8) | scratch;
			return true;
		}

		return false;
	}

	// Jump to Subroutine
	bool NesCpu::JSR() noexcept
	{
		// absolute addressing mode, 6 cycles
		// cycle 3 is nop (internal CPU stuff?)

		// 1    PC     R  fetch opcode, increment PC
		// 3  $0100,S  R  internal operation (predecrement S?)

		switch (step)
		{
		case 2:
			// fetch low address byte
			scratch = readPC();
			break;

		case 4:
			// push PCH on stack
			push(U8(PC >> 8));
			break;

		case 5:
			// push PCL on stack
			push(U8(PC & 255));
			break;

		case 6:
			// copy low address byte to PCL, fetch high address byte to PCH
			PC = (readPC() << 8) | scratch;
			return true;
		}

		return false;
	}

	// Load Accumulator
	bool NesCpu::LDA() noexcept
	{
		using enum ProcessorStatus;

		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			A = scratch;
			if (A == 0)
				P |= Z;
			else
				P &= ~Z;

			if (A & 0b1000'0000)
				P |= N;
			else
				P &= ~N;

			return true;
		}

		return false;
	}

	// Load X Register
	bool NesCpu::LDX() noexcept
	{
		using enum ProcessorStatus;

		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			X = scratch;
			if (X == 0)
				P |= Z;
			else
				P &= ~Z;

			if (X & 0b1000'0000)
				P |= N;
			else
				P &= ~N;

			return true;
		}

		return false;
	}

	// Load Y Register
	bool NesCpu::LDY() noexcept
	{
		using enum ProcessorStatus;

		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			Y = scratch;
			if (Y == 0)
				P |= Z;
			else
				P &= ~Z;

			if (Y & 0b1000'0000)
				P |= N;
			else
				P &= ~N;

			return true;
		}

		return false;
	}

	// Logical Shift Right
	bool NesCpu::LSR() noexcept
	{
		using enum ProcessorStatus;

		auto status = read_modify_write();

		// Modifying the accumulator can be completed immediately, no need to go through scratch
		if (status == AddressStatus::accumulator)
		{
			auto result = op::LSR(A, P);
			A = result.ans;
			P = result.flags;
			return true;
		}

		if (status == AddressStatus::complete)
			return true;

		// we can write our answer to scratch anytime after read_complete up to write_ready,
		// the value will be preserved
		// We'll wait until the last moment to write, but it shouldn't matter
		if (status == AddressStatus::write_ready)
		{
			auto result = op::LSR(scratch, P);
			scratch = result.ans;
			P = result.flags;
		}

		return false;
	}

	// No Operation
	bool NesCpu::NOP() noexcept
	{
		// Implied addressing, 2 cycles. But, perhaps unsurprisingly, nothing to do here
		return true;
	}

	// Logical Inclusive OR
	bool NesCpu::ORA() noexcept
	{
		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			auto result = op::ORA(A, scratch, P);
			A = result.ans;
			P = result.flags;
			return true;
		}

		return false;
	}

	// Push Accumulator
	bool NesCpu::PHA() noexcept
	{
		// Implied addressing, 3 cycles.
		// cycle 2 is essentially a nop (dummy read of PC without incrementing it)
		if (step == 3)
		{
			push(A);
			return true;
		}

		return false;
	}

	// Push Processor Status
	bool NesCpu::PHP() noexcept
	{
		using enum ProcessorStatus;

		// Implied addressing, 3 cycles.
		// cycle 2 is essentially a nop (dummy read of PC without incrementing it)
		if (step == 3)
		{
			// We set B as it technically isn't part of the processor status, but indicates whether
			// hardware interrupts (nmi or irq) or software (BRK, PHP) pushed the status to the stack
			push(U8(P | B));
			return true;
		}

		return false;
	}

	// Pull Accumulator
	bool NesCpu::PLA() noexcept
	{
		using enum ProcessorStatus;

		// Implied addressing, 4 cycles
		// cycle 2 is essentially a nop (dummy read of PC without incrementing it)
		// cycle 3 increments S
		// cycle 4 pulls A from stack

		// I'm assuming that incrementing S is not observable, so we won't break this into separate steps
		if (step == 4)
		{
			A = pop();

			if (A == 0)
				P |= Z;
			else
				P &= ~Z;

			if (A & 0b1000'0000)
				P |= N;
			else
				P &= ~N;

			return true;
		}
		return false;
	}

	// Pull Processor Status
	bool NesCpu::PLP() noexcept
	{
		using enum ProcessorStatus;

		// Implied addressing, 4 cycles
		// cycle 2 is essentially a nop (dummy read of PC without incrementing it)
		// cycle 3 increments S
		// cycle 4 pulls P from stack

		// I'm assuming that incrementing S is not observable, so we won't break this into separate steps
		if (step == 4)
		{
			// explicitly set E and unset B when pulling our flags. Neither are actual status flags.
			// E is hardwired to on, and B indicates how the value got on to the stack and doesn't have meaning otherwise
			P = (ProcessorStatus(pop()) | E) & ~B;
			return true;
		}
		return false;
	}

	// Rotate Left
	bool NesCpu::ROL() noexcept
	{
		using enum ProcessorStatus;

		auto status = read_modify_write();

		// Modifying the accumulator can be completed immediately
		if (status == AddressStatus::accumulator)
		{
			auto result = op::ROL(A, P);
			A = result.ans;
			P = result.flags;
			return true;
		}

		if (status == AddressStatus::complete)
			return true;

		// we can write our answer to scratch anytime after read_complete up to write_ready,
		// the value will be preserved
		// We'll wait until the last moment to write, but it shouldn't matter
		if (status == AddressStatus::write_ready)
		{
			auto result = op::ROL(scratch, P);
			scratch = result.ans;
			P = result.flags;
		}

		return false;
	}

	// Rotate Right
	bool NesCpu::ROR() noexcept
	{
		using enum ProcessorStatus;

		auto status = read_modify_write();

		// Modifying the accumulator can be completed immediately
		if (status == AddressStatus::accumulator)
		{
			auto result = op::ROR(A, P);
			A = result.ans;
			P = result.flags;
			return true;
		}

		if (status == AddressStatus::complete)
			return true;

		// we can write our answer to scratch anytime after read_complete up to write_ready,
		// the value will be preserved
		// We'll wait until the last moment to write, but it shouldn't matter
		if (status == AddressStatus::write_ready)
		{
			auto result = op::ROR(scratch, P);
			scratch = result.ans;
			P = result.flags;
		}

		return false;
	}

	// Return from Interrupt
	bool NesCpu::RTI() noexcept
	{
		using enum ProcessorStatus;

		// Implied addressing, 6 cycles
		// step 2 is a dummy read
		// step 3 increments S, which shouldn't be observable

		switch (step)
		{
		case 4:
			// pop P from stack
			// Note: explicitly set E as it is hardwired high.
			// Clear B as it isn't a real flag and is more an artifact of how the interrupt was generated
			P = (ProcessorStatus(pop()) | E) & ~B;
			break;

		case 5:
			// pop PCL from stack
			PC = pop();
			break;

		case 6:
			// pop PCH from stack
			PC = (pop() << 8) | PC;
			return true;
		}

		return false;
	}

	// Return from Subroutine
	bool NesCpu::RTS() noexcept
	{
		// Implied addressing, 6 cycles
		// step 2 is a dummy read
		// step 3 increments S, which shouldn't be observable

		switch (step)
		{
		case 4:
			// pop PCL from stack
			PC = pop();
			break;

		case 5:
			// pop PCH from stack
			PC = (pop() << 8) | PC;
			break;

		case 6:
			// increment PC
			++PC;
			return true;
		}

		return false;
	}

	// Subtract with Carry
	bool NesCpu::SBC() noexcept
	{
		auto status = read();
		if (status == AddressStatus::read_complete)
		{
			auto result = op::SBC(A, scratch, P);
			A = result.ans;
			P = result.flags;
			return true;
		}

		return false;
	}

	// Set Carry Flag
	bool NesCpu::SEC() noexcept
	{
		// Implied addressing, 2 cycles
		using enum ProcessorStatus;

		P |= C;
		return true;
	}

	// Set Decimal Flag
	bool NesCpu::SED() noexcept
	{
		// Implied addressing, 2 cycles
		using enum ProcessorStatus;

		P |= D;
		return true;
	}

	// Set Interrupt Disable
	bool NesCpu::SEI() noexcept
	{
		// Implied addressing, 2 cycles
		using enum ProcessorStatus;

		P |= I;
		return true;
	}

	// Store Accumulator
	bool NesCpu::STA() noexcept
	{
		auto status = write();

		if (status == AddressStatus::complete)
			return true;

		if (status == AddressStatus::write_ready)
			scratch = A;

		return false;
	}

	// Store X Register
	bool NesCpu::STX() noexcept
	{
		auto status = write();

		if (status == AddressStatus::complete)
			return true;

		if (status == AddressStatus::write_ready)
			scratch = X;

		return false;
	}

	// Store Y Register
	bool NesCpu::STY() noexcept
	{
		auto status = write();

		if (status == AddressStatus::complete)
			return true;

		if (status == AddressStatus::write_ready)
			scratch = Y;

		return false;
	}

	// Transfer Accumulator to X
	bool NesCpu::TAX() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		X = A;

		if (X == 0)
			P |= Z;
		else
			P &= ~Z;

		if (X & 0b1000'0000)
			P |= N;
		else
			P &= ~N;

		return true;
	}

	// Transfer Accumulator to Y
	bool NesCpu::TAY() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		Y = A;

		if (Y == 0)
			P |= Z;
		else
			P &= ~Z;

		if (Y & 0b1000'0000)
			P |= N;
		else
			P &= ~N;

		return true;
	}

	// Transfer Stack Pointer to X
	bool NesCpu::TSX() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		X = S;

		if (X == 0)
			P |= Z;
		else
			P &= ~Z;

		if (X & 0b1000'0000)
			P |= N;
		else
			P &= ~N;

		return true;
	}

	// Transfer X to Accumulator
	bool NesCpu::TXA() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		A = X;

		if (A == 0)
			P |= Z;
		else
			P &= ~Z;

		if (A & 0b1000'0000)
			P |= N;
		else
			P &= ~N;

		return true;
	}

	// Transfer X to Stack Pointer
	bool NesCpu::TXS() noexcept
	{
		// Implied addressing mode, 2 cycles
		S = X;
		return true;
	}

	// Transfer Y to Accumulator
	bool NesCpu::TYA() noexcept
	{
		// Implied addressing mode, 2 cycles
		using enum ProcessorStatus;

		A = Y;

		if (A == 0)
			P |= Z;
		else
			P &= ~Z;

		if (A & 0b1000'0000)
			P |= N;
		else
			P &= ~N;

		return true;
	}

	NesCpu::AddressStatus NesCpu::INX(OpType type) noexcept
	{
		switch (step)
		{
		case 2:
			effective_addr = readPC();
			return AddressStatus::pending;

		case 3:
			effective_addr = (effective_addr + X) & 255;
			return AddressStatus::pending;

		case 4:
			scratch = nes->bus().read(effective_addr);
			return AddressStatus::pending;

		case 5:
		{
			auto hi = nes->bus().read((effective_addr + 1) & 255);
			effective_addr = (hi << 8) | scratch;

			if (type == OpType::write)
				return AddressStatus::write_ready;

			return AddressStatus::pending;
		}
		case 6:
			switch (type)
			{
				using enum OpType;
			case read:
			case read_modify_write:
				scratch = nes->bus().read(effective_addr);
				return AddressStatus::read_complete;

			case write:
				nes->bus().write(effective_addr, scratch);
				return AddressStatus::complete;
			}

			CHECK(false, "Should't ever get here...");
			return AddressStatus::complete;

		case 7:
			return AddressStatus::write_ready;

		case 8:
			nes->bus().write(effective_addr, scratch);
			return AddressStatus::complete;
		}

		CHECK(false, "We shouldn't reach here");
		return AddressStatus::complete;
	}

	NesCpu::AddressStatus NesCpu::ZP(OpType type) noexcept
	{
		switch (step)
		{
		case 2:
			effective_addr = readPC();

			if (type == OpType::write)
				return AddressStatus::write_ready;

			return AddressStatus::pending;

		case 3:
			switch (type)
			{
				using enum OpType;
			case read:
			case read_modify_write:
				scratch = nes->bus().read(effective_addr);
				return AddressStatus::read_complete;

			case write:
				nes->bus().write(effective_addr, scratch);
				return AddressStatus::complete;
			}

			CHECK(false, "Should't ever get here...");
			return AddressStatus::complete;

		case 4:
			return AddressStatus::write_ready;

		case 5:
			nes->bus().write(effective_addr, scratch);
			return AddressStatus::complete;
		}

		CHECK(false, "We shouldn't reach here");
		return AddressStatus::complete;
	}

	NesCpu::AddressStatus NesCpu::IMM([[maybe_unused]] OpType type) noexcept
	{
		CHECK(step == 2, "This should only need to be called once");
		CHECK(type == OpType::read, "Can't write to immediate address");

		scratch = readPC();
		return AddressStatus::read_complete;
	}

	NesCpu::AddressStatus NesCpu::ABS(OpType type) noexcept
	{
		switch (step)
		{
		case 2:
			effective_addr = readPC();
			return AddressStatus::pending;

		case 3:
		{
			effective_addr |= U16(readPC() << 8);

			if (type == OpType::write)
				return AddressStatus::write_ready;

			return AddressStatus::pending;
		}

		case 4:
			switch (type)
			{
				using enum OpType;
			case read:
			case read_modify_write:
				scratch = nes->bus().read(effective_addr);
				return AddressStatus::read_complete;

			case write:
				nes->bus().write(effective_addr, scratch);
				return AddressStatus::complete;
			}

			CHECK(false, "Should't ever get here...");
			return AddressStatus::complete;

		case 5:
			return AddressStatus::write_ready;

		case 6:
			nes->bus().write(effective_addr, scratch);
			return AddressStatus::complete;
		}

		CHECK(false, "We shouldn't reach here");
		return AddressStatus::complete;
	}

	// 1      PC       R  fetch opcode, increment PC
	// 2      PC       R  fetch pointer address, increment PC
	// 3    pointer    R  fetch effective address low
	// 4   pointer+1   R  fetch effective address high,
	// 					add Y to low byte of effective address
	// 5   address+Y*  R  read from effective address,
	// 					fix high byte of effective address
	// 6   address+Y   W  write to effective address

	NesCpu::AddressStatus NesCpu::INY(OpType type) noexcept
	{
		switch (step)
		{
		case 2:
			effective_addr = readPC();
			return AddressStatus::pending;

		case 3:
			scratch = nes->bus().read(effective_addr);
			return AddressStatus::pending;

		case 4:
		{
			auto hi = nes->bus().read((effective_addr + 1) & 255) << 8;
			effective_addr = hi | scratch;
			effective_addr += Y;
			effective_addr &= 0xFFFF;

			// we need an extra cycle to "fix" the address if it crosses a page boundary
			// we model this by skipping the next step if we didn't overflow
			if (std::cmp_equal(hi, effective_addr & 0xFF00))
			{
				// indirect reads use an extra cycle to "fix" the address if it overflows. For some reason, write always takes the extra step regardless
				if (type != OpType::write)
					++step;
			}
			return AddressStatus::pending;
		}

		case 5:
		{
			// this is the adjustment step if we crossed a page boundary
			if (type == OpType::write)
				return AddressStatus::write_ready;

			return AddressStatus::pending;
		}

		case 6:
			switch (type)
			{
				using enum OpType;
			case read:
			case read_modify_write:
				scratch = nes->bus().read(effective_addr);
				return AddressStatus::read_complete;

			case write:
				nes->bus().write(effective_addr, scratch);
				return AddressStatus::complete;
			}

			CHECK(false, "Should't ever get here...");
			return AddressStatus::complete;

		case 7:
			return AddressStatus::write_ready;

		case 8:
			nes->bus().write(effective_addr, scratch);
			return AddressStatus::complete;
		}

		CHECK(false, "We shouldn't reach here");
		return AddressStatus::complete;
	}

	NesCpu::AddressStatus NesCpu::ZPX(OpType type) noexcept
	{
		switch (step)
		{
		case 2:
			effective_addr = readPC();
			return AddressStatus::pending;

		case 3:
			effective_addr = (effective_addr + X) & 255;

			if (type == OpType::write)
				return AddressStatus::write_ready;

			return AddressStatus::pending;

		case 4:
			switch (type)
			{
				using enum OpType;
			case read:
			case read_modify_write:
				scratch = nes->bus().read(effective_addr);
				return AddressStatus::read_complete;

			case write:
				nes->bus().write(effective_addr, scratch);
				return AddressStatus::complete;
			}

			CHECK(false, "Should't ever get here...");
			return AddressStatus::complete;

		case 5:
			return AddressStatus::write_ready;

		case 6:
			nes->bus().write(effective_addr, scratch);
			return AddressStatus::complete;
		}

		CHECK(false, "We shouldn't reach here");
		return AddressStatus::complete;
	}

	NesCpu::AddressStatus NesCpu::ZPY(OpType type) noexcept
	{
		switch (step)
		{
		case 2:
			effective_addr = readPC();
			return AddressStatus::pending;

		case 3:
			effective_addr = (effective_addr + Y) & 255;

			if (type == OpType::write)
				return AddressStatus::write_ready;

			return AddressStatus::pending;

		case 4:
			switch (type)
			{
				using enum OpType;
			case read:
			case read_modify_write:
				scratch = nes->bus().read(effective_addr);
				return AddressStatus::read_complete;

			case write:
				nes->bus().write(effective_addr, scratch);
				return AddressStatus::complete;
			}

			CHECK(false, "Should't ever get here...");
			return AddressStatus::complete;

		case 5:
			return AddressStatus::write_ready;

		case 6:
			nes->bus().write(effective_addr, scratch);
			return AddressStatus::complete;
		}

		CHECK(false, "We shouldn't reach here");
		return AddressStatus::complete;
	}

	NesCpu::AddressStatus NesCpu::ABY(OpType type) noexcept
	{
		switch (step)
		{
		case 2:
			effective_addr = readPC();
			return AddressStatus::pending;

		case 3:
		{
			auto hi = readPC() << 8;
			effective_addr |= hi;
			effective_addr += Y;
			effective_addr &= 0xFFFF;

			// if adding the index pushes us to the next page, add a cycle
			// in this implementation, we will skip cycle 4 if we are still in the same page
			// this only applies to reads, writes always have the extra cycle
			if (type == OpType::read && std::cmp_equal(hi, effective_addr & 0xFF00))
				++step;

			return AddressStatus::pending;
		}

		case 4:
			// page correction cycle
			if (type == OpType::write)
				return AddressStatus::write_ready;

			return AddressStatus::pending;

		case 5:
			switch (type)
			{
				using enum OpType;
			case read:
			case read_modify_write:
				scratch = nes->bus().read(effective_addr);
				return AddressStatus::read_complete;

			case write:
				nes->bus().write(effective_addr, scratch);
				return AddressStatus::complete;
			}

			CHECK(false, "Should't ever get here...");
			return AddressStatus::complete;

		case 6:
			return AddressStatus::write_ready;

		case 7:
			nes->bus().write(effective_addr, scratch);
			return AddressStatus::complete;
		}

		CHECK(false, "We shouldn't reach here");
		return AddressStatus::complete;
	}

	NesCpu::AddressStatus NesCpu::ABX(OpType type) noexcept
	{
		switch (step)
		{
		case 2:
			effective_addr = readPC();
			return AddressStatus::pending;

		case 3:
		{
			auto hi = readPC() << 8;
			effective_addr |= hi;
			effective_addr += X;
			effective_addr &= 0xFFFF;

			// if adding the index pushes us to the next page, add a cycle
			// in this implementation, we will skip cycle 4 if we are still in the same page
			// this only applies to reads, writes always have the extra cycle
			if (type == OpType::read && std::cmp_equal(hi, effective_addr & 0xFF00))
				++step;

			return AddressStatus::pending;
		}

		case 4:
			// page correction cycle
			if (type == OpType::write)
				return AddressStatus::write_ready;

			return AddressStatus::pending;

		case 5:
			switch (type)
			{
				using enum OpType;
			case read:
			case read_modify_write:
				scratch = nes->bus().read(effective_addr);
				return AddressStatus::read_complete;

			case write:
				nes->bus().write(effective_addr, scratch);
				return AddressStatus::complete;
			}

			CHECK(false, "Should't ever get here...");
			return AddressStatus::complete;

		case 6:
			return AddressStatus::write_ready;

		case 7:
			nes->bus().write(effective_addr, scratch);
			return AddressStatus::complete;
		}

		CHECK(false, "We shouldn't reach here");
		return AddressStatus::complete;
	}
}
