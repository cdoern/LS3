package main

import (
	"context"
	"fmt"
	"os"
	"syscall"
	"unsafe"

	"github.com/spf13/cobra"
)

var (
	rootCmd = &cobra.Command{
		Use:                   "mkfs.ls3 [options]",
		Long:                  "localized block based storage formatter",
		SilenceUsage:          true,
		SilenceErrors:         true,
		TraverseChildren:      true,
		DisableFlagsInUseLine: true,
		RunE:                  format,
	}
)

type ioctl_data struct {
	cmd       uint64
	key_len   uint64
	value_len uint64
	key       unsafe.Pointer
	value     unsafe.Pointer
	mountno   uint64
}

func format(cmd *cobra.Command, args []string) error {
	if len(args) == 0 {
		return fmt.Errorf("need at least one arg")
	}
	var err error
	deviceArr := []byte(args[0])
	send := ioctl_data{
		cmd:     4,
		key:     unsafe.Pointer(&deviceArr[0]),
		key_len: uint64(len(deviceArr)),
		mountno: 0,
	}
	// go open/create a device and then kernel module can inercept this opening of a /dev
	// open/rm of dev should maybe be done by kernel module
	f, err := os.OpenFile("/dev/ls3", os.O_RDWR, os.ModeCharDevice)
	if err != nil {
		return err
	}

	_, _, errno := syscall.Syscall(
		syscall.SYS_IOCTL,
		uintptr(f.Fd()),
		uintptr(0x0707), // DELETE
		uintptr(unsafe.Pointer(&send)),
	)
	if errno != 0 {
		err = errno
	}
	if err != nil {
		return err
	}
	return nil
}

func Execute() {
	if err := rootCmd.ExecuteContext(context.Background()); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(125)
	}
	os.Exit(0)
}
