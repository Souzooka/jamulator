package jamulator

import (
	"fmt"
	"reflect"
)

/** Function astPrint(indent int, n interface{})
  * Parameters:
  *   int indent, interface{} n
  * Return values:
  *   Void
  * Behavior:
  *   Some type of debug printing (DOCUMENTATION TODO)
  */
func astPrint(indent int, n interface{}) {
	for i := 0; i < indent; i++ {
		fmt.Print(" ")
	}
	fmt.Println(reflect.TypeOf(n))
}

/** Function (ast ProgramAst) Print()
  * Receiver:
  *   ast ProgramAst (./asm6502.y)
  * Parameters:
  *   Void
  * Return values:
  *   Void
  * Behavior:
  *   Some type of debug printing (DOCUMENTATION TODO)
  */
func (ast ProgramAst) Print() {
	for e := ast.List.Front(); e != nil; e = e.Next() {
		astPrint(0, e.Value)
		switch t := e.Value.(type) {
		case *LabeledStatement:
			astPrint(2, t.Label)
			astPrint(2, t.Stmt)
		case *DataStatement:
			for de := t.dataList.Front(); de != nil; de = de.Next() {
				astPrint(2, de.Value)
			}
		}
	}
}
