package main

import (
	"LS3/cmd/ls3"
	"os"
)

func main() {
	parent := rootCmd

	parent.AddCommand(ls3.LS3PutCommand)
	parent.AddCommand(ls3.LS3GetCommand)
	parent.AddCommand(ls3.LS3DeleteCommand)
	Execute()
	os.Exit(0)
}
