# mymuduo

**仿写muduo库网络部分核心代码，实现一个脱离boost库的全部基于C++11的高性能的mymuduo网络项目。**

****

### 主要实现的模块

**Channel类，Poller类以及派生的EpollPoller类，EventLoop类，Thread类，EventLoopThread类，EventLoopThreadPool类(per thread one loop)，Socket类，InetAddress类，Acceptor类，Buffer类，TcpConnection类，以及最终面向用户的TcpServer类**

### 技术方面

**完全脱离boost库，全部都是使用C++11新语法，包括大量的智能指针的使用和function函数对象，lambda表达式以及bind绑定器的使用，以及C++11提供的线程方面的thread，atomicCAS原子类，metux锁，condition_variable条件变量等知识。**

**整个项目采用Cmake编译，另外使用shell脚本简单实用了一键编译**

### 使用注意

- 环境：Linux系统，g++必须支持C++11及以上标准。
- 编译安装：直接运行mymuduo目录下的autobuild.sh文件，autobuild脚本会将整个项目编译，并将代码头文件自动拷贝到系统的/usr/local/include/mymuduo目录下，将生成的lib库直接拷到系统的/usr/lib目录下。
- 示例代码：在安装好mymuduo后，进入mymuduo下的example目录下，执行make就会生成可执行文件。

