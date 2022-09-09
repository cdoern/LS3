package ls3

import (
	"LS3/cmd/common"
	"fmt"

	"github.com/spf13/cobra"
)

var (
	getDescription = `gets data from storage`

	LS3GetCommand = &cobra.Command{
		Use:     "get KEY",
		Short:   "gets data from storage",
		Long:    getDescription,
		RunE:    get,
		Example: "ls3 get 123",
	}
)

func get(cmd *cobra.Command, args []string) error {
	eng, _ := common.NewStorageEngine()
	str, err := eng.GetObject(args[0])
	fmt.Println(string(str))
	return err
}
