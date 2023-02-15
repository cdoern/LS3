struct appendable_data {
    uint64_t key_len;
    uint64_t value_len;
    char *key;
    char *value; // change to void*
};

struct appendable_data all_data[1000000];

static int write_array(char *key, char *value, uint64_t key_len, uint64_t value_len) {
    struct appendable_data append;
    append.key = key;
    append.value = value;
    append.key_len = key_len;
    append.value_len = value_len;
    int loop;
    int place = tracker;
    for(loop = 0; loop < tracker; loop++) {
        printk(KERN_INFO "key, value: %s, %s\n", all_data[loop].key, all_data[loop].value);
        if (all_data[loop].key == NULL) {
            place = loop;
        } else if (strcmp(all_data[loop].key, key) == 0) {
            place = loop;
            break;
        }
    }
    all_data[place] = append;
    return place;
}

static int read_array(struct ioctl_data *data, char *key) {
    int loop = 0;
    for(loop = 0; loop < tracker; loop++) {
        printk(KERN_INFO "key %d of %d key %s\n", loop, tracker, all_data[loop].key);
        if (all_data[loop].key == NULL) {
            continue;
        }
        if(strcmp(all_data[loop].key, key) == 0) {
            printk(KERN_INFO "found key... retrieving\n");
            //int ret_l =0;
            int amountToCopy = all_data[loop].value_len;
            // if want < have... copy = want
            if (data->value_len < all_data[loop].value_len) {
                amountToCopy = data->value_len;
            }
            // set return to what we have
            printk(KERN_INFO "%d\n", amountToCopy);
            data->value_len = all_data[loop].value_len;
            //   if (copy_to_user((void __user*)arg, data, 32)) {
            //         return -EFAULT;
            //}
            //printk(KERN_INFO "%s\n", all_data[loop].value);
            // if (copy_to_user(data.value, all_data[loop].value, amountToCopy)) {
            //   return -EFAULT;
            // }
            //break;
        }
    }
    return 0;
}

static int look_for_blanks(int place, struct ioctl_data *data) {
    // look for blank spot
    // check sizes using the read size and if not big enough, skip
    // when deleting, I kept the lenghts but deleted the data
    uint64_t key_len;
    uint64_t value_len;
    loff_t readPos = 0;
    int ret = 0;
    while(readPos < end_pos) {
        leftover = 0;
        printk(KERN_INFO "%d\n", (int)readPos);
        loff_t beginning = readPos;
        ret = kernel_read(filp, &key_len, 8, &readPos);
        ret = kernel_read(filp, &value_len, 8, &readPos);
        if ((key_len+value_len) < (data->key_len+data->value_len)) {
            readPos += key_len+value_len;
            continue;
        } else if ((key_len+value_len) - 17 > (data->key_len+data->value_len)) {
            // good w/ chunk
            leftover = (key_len+value_len) - (data->key_len+data->value_len);
        }  else if ((key_len+value_len) == (data->key_len+data->value_len)) {
            // good perfectly
            // do nothing
        }   else {
            // bad
            readPos += key_len+value_len;
            continue;
        }
        char *curr_key = kmalloc(1, GFP_USER);
        ret = kernel_read(filp, curr_key, 1, &readPos);
        readPos += key_len+value_len-1;
        //char *zeros = kmalloc(key_len+1, GFP_USER);
        // memset(zeros, '0', key_len);
        //zeros[key_len] = 0;
        if (curr_key[0] == 0) {
            write_pos = beginning; // or something? make a string of zeros and if our key is that then we got a writing place
            break;
        }
        // ret = kernel_read(filp, curr_value, value_len, &write_pos);
    }
    return ret;
}

