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

