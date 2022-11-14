package LS3_test

import (
	"testing"

	. "github.com/onsi/ginkgo/v2"
	. "github.com/onsi/gomega"
)

func TestLS3(t *testing.T) {
	RegisterFailHandler(Fail)
	RunSpecs(t, "LS3 Suite")
}
