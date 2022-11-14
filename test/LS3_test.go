package LS3_test

import (
	"LS3/ls3"
	"os/exec"

	. "github.com/onsi/ginkgo/v2"
	. "github.com/onsi/gomega"
)

// rmmod && dd
// insmod

var _ = Describe("LS3", func() {
	BeforeEach(func() {
		By("setting up test")
		cmd := exec.Command("sudo", "rmmod", "ls3")
		By(cmd.Run().Error())

		cmd = exec.Command("dd", "if=/dev/zero", "of=/home/charliedoern/Documents/testing.txt", "bs=1000", "count=1000")
		err := cmd.Run()
		if err != nil {
			GinkgoWriter.Println(err.Error())
			Fail(err.Error())
		}

		cmd = exec.Command("sudo", "insmod", "/home/charliedoern/Documents/LS3/ls3.ko")
		err = cmd.Run()
		if err != nil {
			Fail(err.Error())
		}
	})

	AfterEach(func() {

	})
	It("generic test", func() {
		err := ls3.PutObject("123", []byte("testing"))
		Expect(err).Should(BeNil())
		value, err := ls3.GetObject("123")
		Expect(value).Should(Equal([]byte("testing")))
	})

})
