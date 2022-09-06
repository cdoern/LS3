package entities

// StorageEngine defines the functions for the LS3 API
type StorageEngine interface {
	CopyObject()
	DeleteObject()
	GetObject()
	ListObject()
	PutObject()
}
