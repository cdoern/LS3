package ls3

import (
	"LS3/cmd/common"

	"github.com/spf13/cobra"
)

var (
	deleteDescription = `deletes data from storage`

	LS3DeleteCommand = &cobra.Command{
		Use:     "delete KEY",
		Short:   "deletes data from storage",
		Long:    deleteDescription,
		RunE:    delete,
		Example: "ls3 delete 123",
	}
)

func delete(cmd *cobra.Command, args []string) error {
	eng, _ := common.NewStorageEngine()
	err := eng.DeleteObject(args[0])
	return err
}
