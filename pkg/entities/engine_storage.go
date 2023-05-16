package entities

// StorageEngine defines the functions for the LS3 API
type StorageEngine interface {
	CopyObject() error
	DeleteObject(key string) error
	GetObject(key string) ([]byte, error)
	ListObjects() error
	PutObject(key string, value []byte) error
	Format(device string) error
}
