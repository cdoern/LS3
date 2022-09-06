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
		Example: "korero discord login 12345",
	}
)

func put(cmd *cobra.Command, args []string) error {
	return nil
}
