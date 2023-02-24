//go:build linux

package native

import (
	"LS3/ls3"
)

func (s *StorageEngine) CopyObject() error {
	return nil
}

func (s *StorageEngine) DeleteObject(key string) error {
	return ls3.DeleteObject(key)
}

func (s *StorageEngine) GetObject(key string) ([]byte, error) {
	return ls3.GetObject(key)
}

func (s *StorageEngine) ListObjects() error {
	return nil
}

func (s *StorageEngine) PutObject(key string, value []byte) error {
	return ls3.PutObject(key, value)
}

func (s *StorageEngine) Format(device string) error {
	return nil
}
