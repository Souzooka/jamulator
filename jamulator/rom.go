package jamulator

import (
	"bufio"
	"errors"
	"io"
	"os"
	"path"
	"fmt"
	"encoding/binary"
)

const (
	NtscTv = iota
	PalTv
	DualCompatTv
)

type TvSystem int

func (tvs TvSystem) String() string {
	if tvs == NtscTv {
		return "NTSC"
	} else if tvs == PalTv {
		return "PAL"
	}
	return "DualCompatible"
}

const (
	HorizontalMirroring = iota
	VerticalMirroring
	FourScreenVRamMirroring
)

type Mirroring int

func (m Mirroring) String() string {
	if m == HorizontalMirroring {
		return "Horizontal"
	} else if m == VerticalMirroring {
		return "Vertical"
	}
	return "FourScreenVRAM"
}

type Rom struct {
	Filename string
	PrgRom   [][]byte
	ChrRom   [][]byte

	Mapper        byte
	Mirroring     Mirroring
	BatteryBacked bool
	TvSystem      TvSystem
	SRamPresent   bool
}

func Load(ioreader io.Reader) (*Rom, error) {
	reader := bufio.NewReader(ioreader)
	r := new(Rom)
	fmt.Printf("read it")

	// read the header
	buf := make([]byte, 16)
	_, err := io.ReadAtLeast(reader, buf, 16)
	if err != nil {
		return nil, err
	}
	if string(buf[0:4]) != "NES\x1a" {
		return nil, errors.New("Invalid ROM file")
	}
	prgBankCount := int(buf[4])
	chrBankCount := int(buf[5])
	flags6 := buf[6]
	flags7 := buf[7]
	if buf[8] != 0 && buf[8] != 1 {
		return nil, errors.New("Only 8KB program RAM supported")
	}
	flags9 := buf[9]
	flags10 := buf[10]

	r.Mapper = (flags6 >> 4) | (flags7 & 0xf0)
	if flags6&0x8 != 0 {
		r.Mirroring = FourScreenVRamMirroring
	} else if flags6&0x1 != 0 {
		r.Mirroring = HorizontalMirroring
	} else {
		r.Mirroring = VerticalMirroring
	}
	if flags6&0x2 != 0 {
		r.BatteryBacked = true
	}
	if flags6&0x4 != 0 {
		return nil, errors.New("Trainer unsupported")
	}
	if flags7&0x1 != 0 {
		return nil, errors.New("VS Unisystem unsupported")
	}
	if flags7&0x2 != 0 {
		return nil, errors.New("PlayChoice-10 unsupported")
	}
	if (flags7>>2)&0x2 != 0 {
		return nil, errors.New("NES 2.0 format unsupported")
	}
	if flags9&0x1 != 0 {
		return nil, errors.New("PAL unsupported")
	}
	switch flags10 & 0x2 {
	case 0:
		r.TvSystem = NtscTv
	case 2:
		r.TvSystem = PalTv
	default:
		r.TvSystem = DualCompatTv
	}
	r.SRamPresent = flags10&0x10 == 0
	if flags10&0x20 != 0 {
		return nil, errors.New("bus conflicts unsupported")
	}

	r.PrgRom = make([][]byte, prgBankCount)
	fmt.Printf("read at least %v",prgBankCount)
	for i := 0; i < prgBankCount; i++ {
		bank := make([]byte, 0x4000)
		_, err := io.ReadAtLeast(reader, bank, len(bank))
		if err != nil {
			return nil, err
		}
		r.PrgRom[i] = bank
	}

	r.ChrRom = make([][]byte, chrBankCount)
	for i := 0; i < chrBankCount; i++ {
		bank := make([]byte, 0x2000)
		_, err := io.ReadAtLeast(reader, bank, len(bank))
		if err != nil {
			return nil, err
		}
		r.ChrRom[i] = bank
	}

	return r, nil
}

func LoadFile(filename string) (*Rom, error) {
	fd, err := os.Open(filename)
	if err != nil {
		return nil, err
	}

	r, err := Load(fd)
	err2 := fd.Close()
	if err != nil {
		return nil, err
	}
	if err2 != nil {
		return nil, err2
	}
	r.Filename = path.Base(filename)

	return r, nil
}

//dont support mappers, assume 2 banks
func (r *Rom) Read(a int) byte{
	if a >= 0xC000 {
		return r.PrgRom[1][a&0x3FFF]
	}

	return r.PrgRom[0][a&0x3FFF]
}

//dont support mappers, assume 2 banks
func (r *Rom) ReadWord(a int) uint16{
	return binary.LittleEndian.Uint16([]byte{r.Read(a), r.Read(a+1)})
}

func (r *Rom) Save(writer io.Writer) error {
	w := bufio.NewWriter(writer)
	flags6 := byte(0)
	flags7 := byte(0)
	flags9 := byte(0)
	flags10 := byte(0)

	// mapper number
	flags6 |= (r.Mapper & 0x0f) << 4
	flags7 |= r.Mapper & 0xf0

	// mirroring
	switch r.Mirroring {
	case HorizontalMirroring:
		flags6 |= 0x1
	case VerticalMirroring: // nothing to do
	case FourScreenVRamMirroring:
		flags6 |= 0x8
	default:
		panic("Unknown mirroring")
	}

	if r.BatteryBacked {
		flags6 |= 0x2
	}

	switch r.TvSystem {
	case PalTv:
		flags9 |= 0x1
		flags10 |= 0x2
	case NtscTv: // nothing to do
	case DualCompatTv:
		flags10 |= 0x3
	default:
		panic("unknown tv system")
	}

	if !r.SRamPresent {
		flags10 |= 0x10
	}

	header := []byte{
		'N', 'E', 'S', 0x1a,
		byte(len(r.PrgRom)),
		byte(len(r.ChrRom)),
		flags6,
		flags7,
		0,
		flags9,
		flags10,
		0, 0, 0, 0, 0,
	}
	_, err := w.Write(header)
	if err != nil {
		return err
	}

	for _, bank := range r.PrgRom {
		_, err := w.Write(bank)
		if err != nil {
			return err
		}
	}

	for _, bank := range r.ChrRom {
		_, err := w.Write(bank)
		if err != nil {
			return err
		}
	}

	w.Flush()
	return nil
}

func (r *Rom) SaveFile(dir string) error {
	fd, err := os.Create(path.Join(dir, r.Filename))
	if err != nil {
		return err
	}
	err = r.Save(fd)
	err2 := fd.Close()
	if err != nil {
		return err
	}
	if err2 != nil {
		return err2
	}
	return nil
}
