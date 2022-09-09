//go:build linux

package ls3

import (
	"LS3/pkg/entities"
	"fmt"
	"os"
	"syscall"
	"unsafe"
)

type ioctl_data struct {
	key_len   uint64
	value_len uint64
	key       unsafe.Pointer
	value     unsafe.Pointer
}

/*
type ioctl_data struct {
	msg    uintptr
	amount uint32
}
*/

func PutObject(key string, value []byte) error {
	// so here we are going to either...

	// 1) exec.Command() int a c wrapper and do all of the heavy lifting here
	// 2) use ioctl and our kernel module will handle the rest

	keyArr := []byte(key)
	valueArr := value
	//pass len in a struct: ptr to arr of bytes (end in 0), ptr to arr of bytes, len of val
	//concat := [2]string{key, value}
	send := ioctl_data{
		key_len:   uint64(len(key)),
		key:       unsafe.Pointer(&keyArr[0]),
		value_len: uint64(len(value)),
		value:     unsafe.Pointer(&valueArr[0]),
	}
	fmt.Println(unsafe.Sizeof(send))
	// go open/create a device and then kernel module can inercept this opening of a /dev
	// open/rm of dev should maybe be done by kernel module
	f, err := os.OpenFile("/dev/ls3", os.O_RDWR, os.ModeCharDevice)
	if err != nil {
		return err
	}

	_, _, errno := syscall.Syscall(
		syscall.SYS_IOCTL,
		uintptr(f.Fd()),
		uintptr(0x0707), // RDWR
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

func GetObject(key string) ([]byte, error) {
	var err error
	keyArr := []byte(key)
	valueArr := make([]byte, 1000)
	//pass len in a struct: ptr to arr of bytes (end in 0), ptr to arr of bytes, len of val
	//concat := [2]string{key, value}
	send := ioctl_data{
		key_len:   uint64(len(key)),
		key:       unsafe.Pointer(&keyArr[0]),
		value_len: uint64(1000),
		value:     unsafe.Pointer(&valueArr[0]),
	}
	// go open/create a device and then kernel module can inercept this opening of a /dev
	// open/rm of dev should maybe be done by kernel module
	f, err := os.OpenFile("/dev/ls3", os.O_RDWR, os.ModeCharDevice)
	if err != nil {
		return nil, err
	}

	_, _, errno := syscall.Syscall(
		syscall.SYS_IOCTL,
		uintptr(f.Fd()),
		uintptr(0x0001), // READ
		uintptr(unsafe.Pointer(&send)),
	)
	if errno != 0 {
		err = errno
	}
	if err != nil {
		return nil, err
	}

	// REXEC WITH PROPER SIZE
	// think of how you want to store data... tree?
	// data struct on disk? simple approach ... keys= block 0, values = 1-99

	// we passed too much data
	if send.value_len < 1000 {
		_, _, errno := syscall.Syscall(
			syscall.SYS_IOCTL,
			uintptr(f.Fd()),
			uintptr(0x0001), // READ
			uintptr(unsafe.Pointer(&send)),
		)
		if errno != 0 {
			err = errno
		}
		if err != nil {
			return nil, err
		}
	} else {
		valueArr = make([]byte, send.value_len)
		send.value = unsafe.Pointer(&valueArr[0])
		_, _, errno := syscall.Syscall(
			syscall.SYS_IOCTL,
			uintptr(f.Fd()),
			uintptr(0x0001), // READ
			uintptr(unsafe.Pointer(&send)),
		)
		if errno != 0 {
			err = errno
		}
		if err != nil {
			return nil, err
		}
		// we needed to truncate in the kernel
		//err = fmt.Errorf("value was turncated, should have been %d bytes", send.value_len)
	}
	return valueArr, err
}

func CopyObject(srcKey, destKey string) error {
	return nil
}

func ListObjects(keys []string) ([]entities.ListResponse, error) {
	return nil, nil
}

// maybe replace with count and retr[ind] (stat)

func DeleteObject(key string) error {
	var err error
	keyArr := []byte(key)
	valueArr := make([]byte, 1000)
	//pass len in a struct: ptr to arr of bytes (end in 0), ptr to arr of bytes, len of val
	//concat := [2]string{key, value}
	send := ioctl_data{
		key_len:   uint64(len(key)),
		key:       unsafe.Pointer(&keyArr[0]),
		value_len: uint64(1000),
		value:     unsafe.Pointer(&valueArr[0]),
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
		uintptr(0x8000), // DELETE
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
