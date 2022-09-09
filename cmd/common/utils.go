package common

import (
	"LS3/pkg/entities"
	"LS3/pkg/infra"
)

func NewStorageEngine() (entities.StorageEngine, error) {
	return infra.NewStorageEngine()
}
