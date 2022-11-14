package entities

import "time"

type ListResponse struct {
	Key          string
	Size         uint32
	ModifiedTime time.Time
}
