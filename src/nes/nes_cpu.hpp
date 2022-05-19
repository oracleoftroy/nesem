#pragma once

#include <nes_types.hpp>

namespace nesem
{
	class Nes;

	// The NES used a Ricoh 2A03, a custom 6502 CPU with decimal mode disabled and an integrated audio processing unit
	class NesCpu final
	{
	public:
		explicit NesCpu(Nes *nes) noexcept;

		// signals
		void reset(U16 pc_addr = 0) noexcept;
		void nmi() noexcept;
		void dma(U8 page) noexcept;

		bool clock() noexcept;

	private:
		// helper functions

		// push a value onto the stack
		void push(U8 value) noexcept;

		// pop a value off the stack
		U8 pop() noexcept;

		// read and advance the program counter
		U8 readPC() noexcept;

		bool branch(bool condition) noexcept;

		bool interrupt_requested() noexcept;

	private:
		Nes *nes;

		// 6502 registers
		U16 PC = 0; // program counter
		U8 S = 0; // stack pointer, starts from the top of page 1 and grows downward
		ProcessorStatus P; // processor status
		U8 A; // Accumulator
		U8 X; // Index register X
		U8 Y; // Index register Y

		// other state

		// cycles since startup/reset
		U64 cycles = 0;
		int instruction = -1; // the current instruction being executed, or -1 for reset
		U8 step = 0; // the current step for the current instruction
		U8 scratch = 0xFF; // place for instructions to store intermediate value
		U16 effective_addr = 0xFEFE; // temporary address for address modes

		bool nmi_requested = false;
		bool in_dma = false;
		U8 dma_page = 0;
		int dma_step = -1;

	public:
		// All CPU instructions
		bool xxx() noexcept; // invalid instruction
		bool ADC() noexcept;
		bool AND() noexcept;
		bool ASL() noexcept;
		bool BCC() noexcept;
		bool BCS() noexcept;
		bool BEQ() noexcept;
		bool BIT() noexcept;
		bool BMI() noexcept;
		bool BNE() noexcept;
		bool BPL() noexcept;
		bool BRK() noexcept;
		bool BVC() noexcept;
		bool BVS() noexcept;
		bool CLC() noexcept;
		bool CLD() noexcept;
		bool CLI() noexcept;
		bool CLV() noexcept;
		bool CMP() noexcept;
		bool CPX() noexcept;
		bool CPY() noexcept;
		bool DEC() noexcept;
		bool DEX() noexcept;
		bool DEY() noexcept;
		bool EOR() noexcept;
		bool INC() noexcept;
		bool INX() noexcept;
		bool INY() noexcept;
		bool JMP() noexcept;
		bool JSR() noexcept;
		bool LDA() noexcept;
		bool LDX() noexcept;
		bool LDY() noexcept;
		bool LSR() noexcept;
		bool NOP() noexcept;
		bool ORA() noexcept;
		bool PHA() noexcept;
		bool PHP() noexcept;
		bool PLA() noexcept;
		bool PLP() noexcept;
		bool ROL() noexcept;
		bool ROR() noexcept;
		bool RTI() noexcept;
		bool RTS() noexcept;
		bool SBC() noexcept;
		bool SEC() noexcept;
		bool SED() noexcept;
		bool SEI() noexcept;
		bool STA() noexcept;
		bool STX() noexcept;
		bool STY() noexcept;
		bool TAX() noexcept;
		bool TAY() noexcept;
		bool TSX() noexcept;
		bool TXA() noexcept;
		bool TXS() noexcept;
		bool TYA() noexcept;

	private:
		enum class OpType
		{
			read,
			read_modify_write,
			write,
		};

		enum class AddressStatus
		{
			// the operation is ongoing
			pending,

			// the read operation is complete, and scratch holds the read value
			read_complete,

			// the operation is ready to store the result next cycle, make sure scratch is up to date
			write_ready,

			// the operation is complete
			complete,

			// we are modifying the accumulator directly, so we already have everything we need
			accumulator,
		};

		// Addressing modes
		AddressStatus INX(OpType type) noexcept; // read and write
		AddressStatus ZP(OpType type) noexcept;
		AddressStatus IMM(OpType type) noexcept; // read only
		AddressStatus ABS(OpType type) noexcept;
		AddressStatus INY(OpType type) noexcept; // read and write
		AddressStatus ZPX(OpType type) noexcept;
		AddressStatus ZPY(OpType type) noexcept; // used by LDX and STX
		AddressStatus ABY(OpType type) noexcept;
		AddressStatus ABX(OpType type) noexcept;

		AddressStatus read() noexcept;
		AddressStatus write() noexcept;
		AddressStatus read_modify_write() noexcept;

		void log_instruction() noexcept;
	};

}
