修复8.0版本第一次接收的IP地址不准确问题，主要是没有对变量初始化造成的。

8.1版本新增两个hash映射表，用于限制服务器的每IP最大连接数