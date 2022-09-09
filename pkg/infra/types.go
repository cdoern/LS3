package infra

import (
	"LS3/pkg/entities"
	"LS3/pkg/infra/native"
	"context"
)

func NewStorageEngine() (entities.StorageEngine, error) {
	return &native.StorageEngine{Context: context.Background()}, nil
}
