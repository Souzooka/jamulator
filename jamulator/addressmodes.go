package jamulator

type AddrMode int

const (
	nilAddr AddrMode = iota
	absAddr
	absXAddr
	absYAddr
	immedAddr
	impliedAddr
	indirectAddr
	xIndexIndirectAddr
	indirectYIndexAddr
	relativeAddr
	zeroPageAddr
	zeroXIndexAddr
	zeroYIndexAddr

	addrModeCount
)

type opCodeData struct {
	opName   string
	addrMode AddrMode
}

var opNameToOpCode [addrModeCount]map[string]byte

var opCodeDataMap = []opCodeData{
	// 0x00
	{"brk", impliedAddr},
	{"ora", xIndexIndirectAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"ora", zeroPageAddr},
	{"asl", zeroPageAddr},
	{"", nilAddr},
	{"php", impliedAddr},
	{"ora", immedAddr},
	{"asl", impliedAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"ora", absAddr},
	{"asl", absAddr},
	{"", nilAddr},

	// 0x10
	{"bpl", relativeAddr},
	{"ora", indirectYIndexAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"ora", zeroXIndexAddr},
	{"asl", zeroXIndexAddr},
	{"", nilAddr},
	{"clc", impliedAddr},
	{"ora", absYAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"ora", absXAddr},
	{"asl", absXAddr},
	{"", nilAddr},

	// 0x20
	{"jsr", absAddr},
	{"and", xIndexIndirectAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"bit", zeroPageAddr},
	{"and", zeroPageAddr},
	{"rol", zeroPageAddr},
	{"", nilAddr},
	{"plp", impliedAddr},
	{"and", immedAddr},
	{"rol", impliedAddr},
	{"", nilAddr},
	{"bit", absAddr},
	{"and", absAddr},
	{"rol", absAddr},
	{"", nilAddr},

	// 0x30
	{"bmi", relativeAddr},
	{"and", indirectYIndexAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"and", zeroXIndexAddr},
	{"rol", zeroXIndexAddr},
	{"", nilAddr},
	{"sec", impliedAddr},
	{"and", absYAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"and", absXAddr},
	{"rol", absXAddr},
	{"", nilAddr},

	// 0x40
	{"rti", impliedAddr},
	{"eor", xIndexIndirectAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"eor", zeroPageAddr},
	{"lsr", zeroPageAddr},
	{"", nilAddr},
	{"pha", impliedAddr},
	{"eor", immedAddr},
	{"lsr", impliedAddr},
	{"", nilAddr},
	{"jmp", absAddr},
	{"eor", absAddr},
	{"lsr", absAddr},
	{"", nilAddr},

	// 0x50
	{"bvc", relativeAddr},
	{"eor", indirectYIndexAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"eor", zeroXIndexAddr},
	{"lsr", zeroXIndexAddr},
	{"", nilAddr},
	{"cli", impliedAddr},
	{"eor", absYAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"eor", absXAddr},
	{"lsr", absXAddr},
	{"", nilAddr},

	// 0x60
	{"rts", impliedAddr},
	{"adc", xIndexIndirectAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"adc", zeroPageAddr},
	{"ror", zeroPageAddr},
	{"", nilAddr},
	{"pla", impliedAddr},
	{"adc", immedAddr},
	{"ror", impliedAddr},
	{"", nilAddr},
	{"jmp", indirectAddr},
	{"adc", absAddr},
	{"ror", absAddr},
	{"", nilAddr},

	// 0x70
	{"bvs", relativeAddr},
	{"adc", indirectYIndexAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"adc", zeroXIndexAddr},
	{"ror", zeroXIndexAddr},
	{"", nilAddr},
	{"sei", impliedAddr},
	{"adc", absYAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"adc", absXAddr},
	{"ror", absXAddr},
	{"", nilAddr},

	// 0x80
	{"", nilAddr},
	{"sta", xIndexIndirectAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"sty", zeroPageAddr},
	{"sta", zeroPageAddr},
	{"stx", zeroPageAddr},
	{"", nilAddr},
	{"dey", impliedAddr},
	{"", nilAddr},
	{"txa", impliedAddr},
	{"", nilAddr},
	{"sty", absAddr},
	{"sta", absAddr},
	{"stx", absAddr},
	{"", nilAddr},

	// 0x90
	{"bcc", relativeAddr},
	{"sta", indirectYIndexAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"sty", zeroXIndexAddr},
	{"sta", zeroXIndexAddr},
	{"stx", zeroYIndexAddr},
	{"", nilAddr},
	{"tya", impliedAddr},
	{"sta", absYAddr},
	{"txs", impliedAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"sta", absXAddr},
	{"", nilAddr},
	{"", nilAddr},

	// 0xa0
	{"ldy", immedAddr},
	{"lda", xIndexIndirectAddr},
	{"ldx", immedAddr},
	{"", nilAddr},
	{"ldy", zeroPageAddr},
	{"lda", zeroPageAddr},
	{"ldx", zeroPageAddr},
	{"", nilAddr},
	{"tay", impliedAddr},
	{"lda", immedAddr},
	{"tax", impliedAddr},
	{"", nilAddr},
	{"ldy", absAddr},
	{"lda", absAddr},
	{"ldx", absAddr},
	{"", nilAddr},

	// 0xb0
	{"bcs", relativeAddr},
	{"lda", indirectYIndexAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"ldy", zeroXIndexAddr},
	{"lda", zeroXIndexAddr},
	{"ldx", zeroYIndexAddr},
	{"", nilAddr},
	{"clv", impliedAddr},
	{"lda", absYAddr},
	{"tsx", impliedAddr},
	{"", nilAddr},
	{"ldy", absXAddr},
	{"lda", absXAddr},
	{"ldx", absYAddr},
	{"", nilAddr},

	// 0xc0
	{"cpy", immedAddr},
	{"cmp", xIndexIndirectAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"cpy", zeroPageAddr},
	{"cmp", zeroPageAddr},
	{"dec", zeroPageAddr},
	{"", nilAddr},
	{"iny", impliedAddr},
	{"cmp", immedAddr},
	{"dex", impliedAddr},
	{"", nilAddr},
	{"cpy", absAddr},
	{"cmp", absAddr},
	{"dec", absAddr},
	{"", nilAddr},

	// 0xd0
	{"bne", relativeAddr},
	{"cmp", indirectYIndexAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"cmp", zeroXIndexAddr},
	{"dec", zeroXIndexAddr},
	{"", nilAddr},
	{"cld", impliedAddr},
	{"cmp", absYAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"cmp", absXAddr},
	{"dec", absXAddr},
	{"", nilAddr},

	// 0xe0
	{"cpx", immedAddr},
	{"sbc", xIndexIndirectAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"cpx", zeroPageAddr},
	{"sbc", zeroPageAddr},
	{"inc", zeroPageAddr},
	{"", nilAddr},
	{"inx", impliedAddr},
	{"sbc", immedAddr},
	{"nop", impliedAddr},
	{"", nilAddr},
	{"cpx", absAddr},
	{"sbc", absAddr},
	{"inc", absAddr},
	{"", nilAddr},

	// 0xf0
	{"beq", relativeAddr},
	{"sbc", indirectYIndexAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"sbc", zeroXIndexAddr},
	{"inc", zeroXIndexAddr},
	{"", nilAddr},
	{"sed", impliedAddr},
	{"sbc", absYAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"", nilAddr},
	{"sbc", absXAddr},
	{"inc", absXAddr},
	{"", nilAddr},
}

/** Function init()
  * Parameters:
  *   Void
  * Return values:
  *   Void
  * Behavior:
  *   DOCUMENTATION TODO
  */
func init() {
	for i := 0; i < int(addrModeCount); i++ {
		opNameToOpCode[i] = make(map[string]byte)
	}
	for opCode := 0; opCode < 256; opCode++ {
		info := opCodeDataMap[opCode]
		opNameToOpCode[info.addrMode][info.opName] = byte(opCode)
	}
}

