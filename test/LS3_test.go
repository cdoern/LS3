package LS3_test

import (
	"LS3/ls3"
	"math/rand"
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
	It("put and get", func() {
		err := ls3.PutObject("123", []byte("testing"))
		Expect(err).Should(BeNil())
		value, err := ls3.GetObject("123")
		Expect(value).Should(Equal([]byte("testing")))
	})

	It("put multiple objects, delete, and get", func() {
		err := ls3.PutObject("123", []byte("testing"))
		Expect(err).Should(BeNil())
		err = ls3.PutObject("456", []byte("hello"))
		Expect(err).Should(BeNil())
		err = ls3.PutObject("789", []byte("world"))
		Expect(err).Should(BeNil())
		err = ls3.DeleteObject("456")
		Expect(err).Should(BeNil())
		value, err := ls3.GetObject("789")
		Expect(value).Should(Equal([]byte("world")))
	})

	It("stress test", func() {
		var letterRunes = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
		i := int(rand.Int31n(20))

		keys := []string{}
		values := []string{}
		for j := 0; j < i; j++ {
			key := make([]rune, 10)
			val := make([]rune, 10)
			for i := range key {
				key[i] = letterRunes[rand.Intn(len(letterRunes))]
			}
			for i := range val {
				val[i] = letterRunes[rand.Intn(len(letterRunes))]
			}
			err := ls3.PutObject(string(key), []byte(string(val)))
			Expect(err).Should(BeNil())
			keys = append(keys, string(key))
			values = append(values, string(val))
		}

		deleted := make([]bool, i)
		for k := 0; k < i; k++ {
			ind := rand.Int31n(int32(i))
			err := ls3.DeleteObject(keys[ind])
			deleted[ind] = true
			Expect(err).Should(BeNil())
		}

		for l := 0; l < i; l++ {
			ind := int32(-1)
			for ind == -1 || deleted[ind] {
				ind = rand.Int31n(int32(i))
			}
			val, err := ls3.GetObject(keys[ind])
			Expect(err).Should(BeNil())
			Expect(val).Should(Equal(values[ind]))
		}

		err := ls3.PutObject("123", []byte("testing"))
		Expect(err).Should(BeNil())
		err = ls3.PutObject("456", []byte("hello"))
		Expect(err).Should(BeNil())
		err = ls3.PutObject("789", []byte("world"))
		Expect(err).Should(BeNil())
		err = ls3.DeleteObject("456")
		Expect(err).Should(BeNil())
		value, err := ls3.GetObject("789")
		Expect(value).Should(Equal([]byte("world")))
	})

})
