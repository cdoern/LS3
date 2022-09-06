package ls3

import (
	"github.com/spf13/cobra"
)

var (
	putDescription = `puts data into storage`

	LS3PutCommand = &cobra.Command{
		Use:     "put DATA",
		Short:   "put data into storage",
		Long:    putDescription,
		RunE:    put,
		Example: "ls3 put data.txt",
	}
)

func put(cmd *cobra.Command, args []string) error {
	return nil
}
