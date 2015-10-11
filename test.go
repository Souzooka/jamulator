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
	// j,_ := rom.Jit()
	// j.Print();
	// c, err := j.CompileToFilename("test")
	// fmt.Printf("errs: %v %v",err,c.Errors)

	var fl jamulator.CompileFlags
	rom.RecompileToBinary(filename+"x",fl)
}