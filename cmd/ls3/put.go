package ls3

import (
	"LS3/cmd/common"
	"os"

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

var (
	isFile bool
)

func putFlags(cmd *cobra.Command) {
	flags := cmd.Flags()

	fileFlagName := "file"
	flags.BoolVarP(&isFile, fileFlagName, "f", false, "specify insertion of a file rather than a string")

}

func init() {
	putFlags(LS3PutCommand)
}

func put(cmd *cobra.Command, args []string) error {
	eng, _ := common.NewStorageEngine()
	var err error
	val := []byte(args[1])
	name := ""
	if isFile {
		val, err = os.ReadFile(args[1])
		if err != nil {
			return err
		}
		info, err := os.Stat(args[1])
		if err != nil {
			return err
		}
		name = info.Name()
	} else {
		name = args[0]
	}
	err = eng.PutObject(name, val)
	return err
}
