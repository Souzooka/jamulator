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
	fmt.Printf("first instruction %v \n",rom.Read(0xfffc))
	fmt.Printf("first instruction %v \n",rom.Read(0xfffd))
	fmt.Printf("first instruction %v \n",rom.Read(0xfffe))
	fmt.Printf("first instruction %v \n",rom.Read(0xffff))
	rom.MarkAsInstruction(0xfffe)
}