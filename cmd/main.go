package main

import (
	"LS3/cmd/ls3"
	"os"
)

func main() {
	parent := rootCmd

	parent.AddCommand(ls3.LS3PutCommand)
	Execute()
	os.Exit(0)
}
