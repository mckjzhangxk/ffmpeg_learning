# 基础命令
```sh
openssl version
--help

#基础命令，摘要命令，加密命令
openssl help 

```
# 完整性integrity
- 数据完整性，数据有没有传输中发生错误或者被恶意篡改
- 生成摘要，验证摘要
- 三类算法：MD（Message Digest）,SHA(Secure Hash Alg),MAC(message authencication code)
```sh
#验证数据的完整性integrity
openssl sha256 -hex 【输入文件】
#输出到sum.sha256文件中
openssl sha256 -hex -out  sum.sha256【输入文件】
```
## 摘要 如何防止被篡改
- HMAC :Hash based Message Authentication Code
- 问题：攻击的人 可以把 数据，摘要 一起篡改，如何应对？
- 答案：假设通信双方约定了某个secret，没有第三人知道，采用一下方式
```sh
#1.最简单的防篡改技术

# 0 生成双方约定好的secret
openssl rand -out secret.txt -hex 16
#发送方：

# 1.1把secret.txt 追加给earth.txt,然后sha1
echo -n `cat earth.txt` `cat secret.txt`|openssl sha1 -out earth.my.s
# 1.2把earth.txt, earth.my发生给对方

#接受方：
# 也重新生成一遍摘要
echo -n `cat earth.txt` `cat secret.txt`|openssl sha1 -out earth.my.r
# 验证 摘要是否一致
diff earth.my.s earth.my.r


#2.使用hmac，生成 Authentication Code

# 两种写法，使用sha1
openssl sha1 -hmac aabbccdd -out earch.sha1.hmac earth.txt
openssl dgst -sha1 -hmac aabbccdd -out earch.sha1.hmac earth.txt 

# 两种写法，使用sha256
openssl sha256 -hmac aabbccdd -out earch.sha256.hmac earth.txt
openssl dgst -sha256 -hmac aabbccdd -out earch.sha256.hmac earth.txt 

 
```

## 数字签名，防止被冒充
- 如何验证 某个网站给我发的消息，就是来自那个网站呢（这里假设传输的是明文，或者对称密钥泄露，对称私钥没有泄露）？
- 答：假设我有那个网站的pubkey(我确认这个公钥是对的)
- 接受者 收到消息的时候，同时收到了签名（发送端生成的）

```sh
#生成密钥对，私钥在服务器，公钥分发给客户
openssl genrsa -out pri.pem 
openssl rsa -in pri.pem -pubout -out pub.pem   


# 使用rsa算法签名,私钥来签名，服务端
openssl rsautl -sign -in message -inkey pri.pem -out message.rsa.sig          
#公钥来验证签名，客户端
openssl rsautl  -verify -pubin -inkey pub.pem -in message.rsa.sig

#rsa的缺点是 对签名数据大小有限制，我们可以结合摘要算法来完成签名

#使用sha1摘要算法

#发生方生成摘要
openssl sha1 -sign pri.pem  -out message.sha1.sig message
#接收方 验证摘要
openssl sha1  -verify pub.pem -signature message.sha1.sig message

# 使用sha256摘要算法
openssl sha256 -sign pri.pem  -out message.sha256.sig message
openssl sha256  -verify pub.pem -signature message.sha256.sig message


#以上过程等价于 
#1.先生成摘要文件msg.sal
openssl sha256 -hex  -out msg.sal message
#输出 f64cbfa5f10cb68d753fcc881b4722e1feb99f6fa48619d9d4fb9bee4f77bb4b
#2.用rsa 加密msg.sal
openssl rsautl -sign -in msg.sal -inkey pri.pem -out message.rsa.sig  

#3.还是使用以下命令验证
openssl sha256  -verify pub.pem -signature message.sha256.sig message
```
# 证书
- <font color=pink>把 (pubkey,组织信息，有效时间,摘要算法)做个摘要，使用ca机构的 私钥对摘要做【签名】，生成的数据 是证书</font>
- <font color=red>证书其实是明文的</font>
- 客户端通过浏览器内置的 ca的公钥(证书)，验证证书有消息：做法：
- - ca的公钥 解密 【签名】，得到 摘要 d1
- - 对证书(除签名)部分 使用摘要算法，算出d2
- - 判断 d1==d2
```sh
#把 pem证书转换成  der证书
openssl x509 -in cert.pem -out cert.der -outform DER

#查看证书
openssl x509 -in ca.crt -text -noout
```
# 加密解密Confidentinal
## 生成key的方法
```sh
1. randkey
#生成随机的16个字节，单位byte
openssl rand -hex 16

2. keypair
#生成不同位数的rsa key pair，单位bit
openssl genrsa
openssl genrsa 2048
openssl genrsa -out key.pri 1024



# 查看生成的rsa,注意key.pri是私钥，但是包含的可以生成公钥的全部信息
openssl rsa -in key.pri -text -noout
# modulus=(p-1)*(q-1)
# publicExponent=e
# privateExponent=d
# prime1,prime2=p,q
#exponent1?exponent2?coefficient?


#生成对应的pubkey
openssl rsa -in key.pri   -pubout  -out key.pub  
openssl rsa -in key.pub -pubin -text -noout


# 使用以下命令，生成的公钥，私钥都会经过aes-256-cbc再次加密
openssl genrsa -aes-256-cbc - -out key.pem.enc 1024
openssl rsa -aes-256-cbc  -in key.pem.enc  -pubout -out pub.pem.enc

```

```SH

# PEM转DER,PEM是base64格式，DER是二进制格式
# PEM是 base64 加上（BEGIN XXX END XXX),解码后就是DER！！！
openssl rsa -in key.pem -out key.der -outform DER
openssl rsa -in pub.pem -pubin -out pub.der -outform DER
# 查看DER
openssl rsa -in key.der -inform DER  -noout -text
openssl rsa -in pub.der -pubin -inform DER  -noout -text
```
### 生成DSA算法的keypair
```sh
 #先生成参数文件
 openssl dsaparam -out ds.param 1024
 #私钥的生成与查看
 openssl gendsa -out key.ds ds.param
 openssl dsa -in key.ds -text -noout
 #公钥的生成与查看
 openssl dsa -in key.ds -pubout -out pub.ds
 openssl dsa -in pub.ds -pubin -text -noout

 #生成私钥和公钥 加密的文件命令
 openssl gendsa --aes-256-cbc -out key.ds.enc ds.param
 openssl dsa --aes-256-cbc -in  key.ds.enc -pubout -out pub.ds.enc

 openssl dsa --aes-256-cbc -in key.ds.enc -text -noout
```
### 使用genkey，pkey命令
- <font color=red>所有genkey生成的私钥，都是pkcs#8格式，而不是pkcs#1</font>
```sh
# pkeyopt为每种算法设置 参数
openssl genpkey  -algorithm rsa  -pkeyopt rsa_keygen_bits:2048 -out kkk.pem
#查看
openssl pkey -in kkk.pem -text -noout 
```
## 使用不同的ciper加密
### AES
```sh
#查看支持的算法
openssl list -cipher-commands 

#生成key
openssl rand -hex -out my.key 16

#选择ciper block chain,加密
openssl aes-256-cbc -in message -kfile my.key -e -out  message.enc

openssl aes-256-cbc -in message.enc -kfile my.key -d -out  message.dec

# 加上-a，表示输出，输入都是base64 格式
openssl aes-256-cbc -in message -kfile my.key -a -e -out  message.enc
openssl aes-256-cbc -in message.enc -kfile my.key -a -d -out  message.dec


#不使用kfile,而是输入密码，经过多次迭代得到的key更为安全，
#pbkdf2： password based key derive format
openssl aes-256-cbc -in message  -a -e -out  message.enc -pbkdf2 -iter 1000
openssl aes-256-cbc -in message.enc  -a -d -out  message.dec -pbkdf2 -iter 1000

```
### RSA

```sh
#以下命令可以用户加密【对称密钥】
#加密需要告诉 这是pubkey( -pubin)
openssl rsautl  -encrypt -in message -inkey key.pub  -pubin -out message.enc
#解密
openssl rsautl  -decrypt -in message.enc -inkey key.pri  -out message.dec

#与AES不同，rsa不会分块，而是对整个message加密，如果keysize过小会报错。
# message的长度不应该 超过keysize-11
# PKCS#1 
openssl genrsa -out key.pri 1024
openssl rsa -in key.pri   -pubout  -out key.pub  

openssl rand -hex -out data 59 #error
openssl rsautl  -encrypt -in data -inkey key.pub  -pubin -out data.enc

```
