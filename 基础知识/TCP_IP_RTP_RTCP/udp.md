``` 
0      7 8     15 16    23 24    31
+--------+--------+--------+--------+
|     Source Port | Destination Port|
+--------+--------+--------+--------+
|     Length      |    Checksum     |
+--------+--------+--------+--------+
|                                   |
|       Data (Variable Length)      |
|                                   |
+-----------------------------------+
```

Checksum 校验由以下 数据的生成

- udp header+ udp payload
- presude ip header
- - src ip
- - dest ip
- - ip数据包设置的协议（udp=17）
- - Length


```c
//校验和算法，就是每2个字节相加，并且把产生的所有进位也加入到sum中
uint16_t checksum_generic(uint16_t *addr, uint32_t count)
{
    register unsigned long sum = 0;

    for (sum = 0; count > 1; count -= 2)
        sum += *addr++;
    if (count == 1)
        sum += (char)*addr;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16); 

    return ~sum;
}
```