package jamulator

import (
	"bytes"
	"fmt"
	"io/ioutil"
	"testing"
)

type testAsm struct {
	inFile          string
	expectedOutFile string
}

// Declare ROMS for testing
var testAsmList = []testAsm{
	{
		"test/suite6502.asm",
		"test/suite6502.bin.ref",
	},
	{
		"test/zelda.asm",
		"test/zelda.bin.ref",
	},
	{
		"test/hello.asm",
		"test/hello.bin.ref",
	},
}

// Declare ROMS for testing
var testDisAsmList = []string{
	"test/suite6502.bin.ref",
	"test/zelda.bin.ref",
	"test/hello.bin.ref",
}

/** Function TestAsm(t *testing.T)
  * Parameters:
  *   t (DOCUMENTATION TODO) ptr testing.T
  * Return values:
  *   Void
  * Behavior:
  *   DOCUMENTATION TODO
  */
func TestAsm(t *testing.T) {
	for _, ta := range testAsmList {
		expected, err := ioutil.ReadFile(ta.expectedOutFile)
		if err != nil {
			t.Error(err)
		}
		programAst, err := ParseFile(ta.inFile)
		if err != nil {
			t.Error(err)
		}
		program := programAst.ToProgram()
		if len(program.Errors) > 0 {
			t.Error(fmt.Sprintf("%s: unexpected errors", ta.inFile))
		}
		buf := new(bytes.Buffer)
		err = program.Assemble(buf)
		if err != nil {
			t.Error(err)
		}
		if bytes.Compare(buf.Bytes(), expected) != 0 {
			t.Error(fmt.Sprintf("%s: does not match expected output", ta.inFile))
		}
	}
}

/** Function TestDisassembly(t *testing.T)
  * Parameters:
  *   t (DOCUMENTATION TODO) ptr testing.T
  * Return values:
  *   Void
  * Behavior:
  *   DOCUMENTATION TODO
  */
func TestDisassembly(t *testing.T) {
	// try disassembling the ref and reassembling it, it should match byte for byte
	for _, binfile := range testDisAsmList {
		expected, err := ioutil.ReadFile(binfile)
		if err != nil {
			t.Error(err)
		}

		// disassemble binary file into a program
		expectedBuf := bytes.NewBuffer(expected)
		program, err := Disassemble(expectedBuf)
		if err != nil {
			t.Error(err)
		}

		// write the source code into a buffer
		sourceBuf := new(bytes.Buffer)
		err = program.WriteSource(sourceBuf)
		if err != nil {
			t.Error(err)
		}

		// load the source code into a program
		programAst, err := Parse(sourceBuf)
		if err != nil {
			t.Error(err)
		}
		program = programAst.ToProgram()
		if len(program.Errors) > 0 {
			t.Error(fmt.Sprintf("%s: unexpected errors", binfile))
		}

		// assemble the source code into a binary buffer
		binBuf := new(bytes.Buffer)
		err = program.Assemble(binBuf)
		if err != nil {
			t.Error(err)
		}
		if bytes.Compare(binBuf.Bytes(), expected) != 0 {
			t.Error(fmt.Sprintf("%s: does not match expected output", binfile))
		}
	}
}
