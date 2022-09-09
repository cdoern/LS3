package ls3

import (
	"LS3/cmd/common"
	"fmt"
	"net/http"
	"os"

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

func getFlags(cmd *cobra.Command) {
	flags := cmd.Flags()

	fileFlagName := "file"
	flags.BoolVarP(&isFile, fileFlagName, "f", false, "specify retrieval of a file rather than a string")

}

func init() {
	getFlags(LS3GetCommand)
}

func get(cmd *cobra.Command, args []string) error {
	eng, _ := common.NewStorageEngine()
	str, err := eng.GetObject(args[0])
	if isFile {
		contentType := http.DetectContentType(str)
		var f *os.File
		switch contentType {
		case "image/jpeg":
			f, err = os.Create("out.jpeg")
			if err != nil {
				return err
			}
		case "image/png":
			f, err = os.Create("out.png")
			if err != nil {
				return err
			}
		}
		_, err = f.Write(str)
		if err != nil {
			return err
		}
	} else {
		fmt.Println(string(str))
	}
	return err
}
