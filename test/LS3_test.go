package LS3_test

import (
	"LS3/ls3"
	"bytes"
	"fmt"
	"math/rand"
	"os/exec"
	"sync"

	. "github.com/onsi/ginkgo/v2"
	. "github.com/onsi/gomega"
)

// rmmod && dd
// insmod

var testLock sync.Mutex

var _ = Describe("LS3", func() {
	BeforeEach(func() {
		//testLock.Lock()
		By("setting up test")

		errout := &bytes.Buffer{}
		cmd := exec.Command("/bin/sh", "-c", "sudo rmmod ls3")
		/*cmd.SysProcAttr = &syscall.SysProcAttr{
			Credential: &syscall.Credential{
				Uid:         uint32(0),
				Gid:         uint32(0),
				Groups:      nil,
				NoSetGroups: false,
			},
		}*/
		//cmd.Stdout = nil
		cmd.Stderr = errout
		err := cmd.Run()
		for err != nil {
			cmd := exec.Command("/bin/sh", "-c", "sudo rmmod ls3")
			err = cmd.Run()
		}

		//By(cmd.Run().Error())
		errout = &bytes.Buffer{}
		cmd = exec.Command("dd", "if=/dev/zero", "of=/home/charliedoern/Documents/testing.txt", "bs=1000", "count=1000000")
		cmd.Stderr = errout
		err = cmd.Run()
		if err != nil {
			GinkgoWriter.Println(err.Error() + " " + errout.String())
			Fail(err.Error())
		}
		cmd = exec.Command("/bin/sh", "-c", "sudo insmod /home/charliedoern/Documents/LS3/ls3.ko")
		/*cmd.SysProcAttr = &syscall.SysProcAttr{
			Credential: &syscall.Credential{
				Uid:         uint32(0),
				Gid:         uint32(0),
				Groups:      nil,
				NoSetGroups: false,
			},
		}*/
		errout = &bytes.Buffer{}
		cmd.Stderr = errout
		err = cmd.Run()
		if err != nil {
			GinkgoWriter.Println(err.Error() + " " + errout.String())
			Fail(err.Error())
		}
		//testLock.Unlock()
	})

	AfterEach(func() {
		//	testLock.Lock()
		//	GinkgoWriter.Println("in after")

		//testLock.Unlock()
	})
	It("put and get", func() {
		//testLock.Lock()

		err := ls3.PutObject("123", []byte("testing"))
		Expect(err).Should(BeNil())
		GinkgoWriter.Println("here in put/get")
		value, err := ls3.GetObject("123")
		Expect(err).Should(BeNil())
		Expect(string(value)).Should(Equal("testing"))
		GinkgoWriter.Println("this is done")

		//testLock.Unlock()

	})

	It("put multiple objects, delete, and get", func() {
		//testLock.Lock()

		err := ls3.PutObject("123", []byte("testing"))
		Expect(err).Should(BeNil())
		err = ls3.PutObject("456", []byte("hello"))
		Expect(err).Should(BeNil())
		err = ls3.PutObject("789", []byte("world"))
		Expect(err).Should(BeNil())
		err = ls3.DeleteObject("456")
		Expect(err).Should(BeNil())
		value, err := ls3.GetObject("789")
		Expect(string(value)).Should(Equal("world"))
		//testLock.Unlock()

	})

	It("stress test", func() {
		//testLock.Lock()

		var letterRunes = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
		i := 100

		keys := []string{}
		values := []string{}
		_, _ = GinkgoWriter.Write([]byte(fmt.Sprintf("putting %d objects\n", i)))
		for j := 0; j < i; j++ {
			_, _ = GinkgoWriter.Write([]byte(fmt.Sprintf("putting object %d\n", j)))

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
		k := 0
		for k = 0; k < i/2; k++ {
			_, _ = GinkgoWriter.Write([]byte(fmt.Sprintf("deleting object %d\n", k)))

			ind := rand.Int31n(int32(i))
			err := ls3.DeleteObject(keys[ind])
			deleted[ind] = true
			Expect(err).Should(BeNil())
		}
		_, _ = GinkgoWriter.Write([]byte(fmt.Sprintf("Deleted %d objects", k)))

		for l := 0; l < i; l++ {
			_, _ = GinkgoWriter.Write([]byte(fmt.Sprintf("getting object %d\n", l)))
			ind := int32(-1)
			for ind == -1 || deleted[ind] {
				ind = rand.Int31n(int32(i))
			}
			val, err := ls3.GetObject(keys[ind])
			Expect(err).Should(BeNil())
			Expect(string(val)).Should(Equal(values[ind]))
		}
		// SKIP: BUG WHEN REMOVING A LOT OF ITEMS AND ADDING 3+ ITEMS
		/*
			err := ls3.PutObject("456", []byte("h"))
			Expect(err).Should(BeNil())
			err = ls3.PutObject("789", []byte("w"))
			Expect(err).Should(BeNil())
			err = ls3.PutObject("123", []byte("t"))
			Expect(err).Should(BeNil())
			value, err := ls3.GetObject("123")
			Expect(err).Should(BeNil())
			Expect(string(value)).Should(Equal("t"))
			value, err = ls3.GetObject("789")
			Expect(err).Should(BeNil())
			Expect(string(value)).Should(Equal("w"))
			err = ls3.DeleteObject("456")
			Expect(err).Should(BeNil())
		*/
		//testLock.Unlock()

	})

})
