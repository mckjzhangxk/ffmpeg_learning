## DNS服务器构架
- 层次结构，例如www.standford.edu，由 . ,edu , standford,www 四部分组成。
- 最高层 . 对应13个高可用的服务器，称之为rootserver，分布在世界各地。
- rootserver 的ip一般存放在 本地文件系统中。
## DNS Record Type
|||
|-|-|
|A| IPV4地址|
|AAAA|IPV6地址|
|CNAME|别名|
|NS|nameserver,ns中存放的是 域名与ip的对应关系，比如rootserver|
## DNS查询
- Recursive: 类似于lookup命令的查询
- non-recursive:
```sh
#非递归查询 www.stanford.edu的 ip

# 1.问rootserver ,
dig +norec www.stanford.edu @b.root-servers.net 
# 返回如下  edu.的 nameserver是g.edu-servers.net.( 192.42.93.30),
# 向它可以查询 www.stanford 

# ;; AUTHORITY SECTION:
# edu.                    172800  IN      NS      g.edu-servers.net.
# ;; ADDITIONAL SECTION:
# g.edu-servers.net.      172800  IN      A       192.42.93.30


# 2.继续查询 www.standford
dig +norec www.stanford.edu 192.42.93.30
dig +norec www.stanford.edu @g.edu-servers.net.

# 返回 我们知道 向avallone.stanford.edu.继续查询
# avallone.stanford.edu是 nameserver

# ;; AUTHORITY SECTION:
# stanford.edu.           172800  IN      NS      avallone.stanford.edu.
# ;; ADDITIONAL SECTION:
# avallone.stanford.edu.  172800  IN      A       204.63.224.53

# 3.继续查询www
dig +norec www.stanford.edu @204.63.224.53
dig +norec www.stanford.edu @avallone.stanford.edu.

# 返回 我们知道  www.stanford.edu. 有一个别名pantheon-systems.map.fastly.net.

# ;; ANSWER SECTION:
# www.stanford.edu.       1800    IN      CNAME   pantheon-systems.map.fastly.net.

# 4.继续 pantheon-systems.map.fastly.net.
dig +norec pantheon-systems.map.fastly.net.  @b.root-servers.net 
# 返回
# ;; AUTHORITY SECTION:
# net.                    172800  IN      NS      a.gtld-servers.net.
# ;; ADDITIONAL SECTION:
# a.gtld-servers.net.     172800  IN      A       192.5.6.30

# 5.继续查询pantheon-systems.map.fastly
dig +norec pantheon-systems.map.fastly.net. @192.5.6.30
# 返回
# ;; AUTHORITY SECTION:
# fastly.net.             172800  IN      NS      ns1.fastly.net.
# ;; ADDITIONAL SECTION:
# ns1.fastly.net.         172800  IN      A       23.235.32.32

# 6.继续查询 pantheon-systems.map
dig pantheon-systems.map.fastly.net. @23.235.32.32
# 返回,我们知道 www.stanford.edu --> 146.75.114.133
# ;; ANSWER SECTION:
# pantheon-systems.map.fastly.net. 30 IN  A       146.75.114.133
```
- 查询nameserver,有时候可能关心的是 域名服务器(nameserver)，而不是具体的域名ip
```sh
# 查看 sina.com.cn的域名服务器.
dig ns sina.com.cn

#  sina.com.cn下面 管理下面的域名：
#  news.sina.com.cn
#  mail.sina.com.cn
#   .....

```

## Resource Record
- DNS请求,响应中每一条记录都是一个RR
- 格式 <font color=red>name type [class] [ttl] [rdata_length]  [rdata]</font>
- class: IN表示internet
- type: A、AAAA、CNAME，NS等
- rdata_length，rdata 是实际的数据部门
- RR需要被压缩成512个字节

## DNS 数据包结构
- 请求头：必须设置
```c
struct DnsHeader {
    uint16_t id;
    uint16_t opts;      //一些选项，其中最重要的是第八位，表示是否recurisive查询
    uint16_t qdcount;   //query record 的数目
    uint16_t ancount;   //answer record 的数目
    uint16_t nscount;   //nameserver record 的数目
    uint16_t arcount;   //additonal record 的数目
};
```
- 0：N个 Question:每个都是一条RR
```c
// name type class
struct DnsQuestion {
    char  name[N]        //长度是N
    uint16_t qtype;     //query type,1-A 类型查询
    uint16_t qclass;    //1-Internet
};

```
- 0：N个 Answer:每个都是一条RR
- 0：N个 Authority:每个都是一条RR
- 0：N个 Additional:每个都是一条RR

### 关于RR.name的格式与压缩

- 格式， 把.的位置替换成下一个字符到下一个.之间的长度
```sh
# 例如
pantheon-systems.map.fastly.net
# ==>
[16]pantheon-systems[3]]map[6]fastly[3]net
```

## 补充
```sh
# 向nameserver 192.168.1.211:23451 来解析地址
dig www.stanford.edu @192.168.1.211 -p 23451
```