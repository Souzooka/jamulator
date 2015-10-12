package main

import (
	"github.com/badfortrains/jamulator/jamulator"
	"flag"
	"fmt"
	"os"
)

func main(){
	flag.Parse()
	filename := flag.Arg(0)
	rom, err := jamulator.LoadFile(filename)
	if err != nil {
		fmt.Fprintf(os.Stderr, "%s\n", err.Error())
		os.Exit(1)
	}
	//j,_ := rom.Jit()
	//j.Print();

	// c, err := j.CompileToFilename("test")
	// fmt.Printf("errs: %v %v",err,c.Errors)
	// fmt.Printf("jumpcount: %v",j.JumpCount)

	var flags jamulator.CompileFlags
	flags |= jamulator.IncludeDebugFlag
	rom.RecompileToBinary(filename+"123",flags)
}