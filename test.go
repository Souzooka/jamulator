package main

import (
	"fmt"
	"github.com/andrewrk/jamulator/jamulator"
	"flag"
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
	j,_ := rom.Jit()
	j.Print();
}