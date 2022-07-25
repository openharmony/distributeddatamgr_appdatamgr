# 首选项（Preferences）

## 简介

**首选项（Preferences）** 主要提供轻量级Key-Value操作，支持本地应用存储少量数据，数据存储在本地文件中，同时也加载在内存中，所以访问速度更快，效率更高。首选项提供非关系型数据存储，不宜存储大量数据，经常用于操作键值对形式数据的场景。

1.  本模块提供首选项的操作类，应用通过这些操作类完成首选项操作。
2.  借助getPreferences，可以将指定文件的内容加载到Preferences实例，每个文件最多有一个Preferences实例，系统会通过静态容器将该实例存储在内存中，直到主动从内存中移除该实例或者删除该文件。
3.  获取Preferences实例后，可以借助Preferences类的函数，从Preferences实例中读取数据或者将数据写入Preferences实例，通过flush将Preferences实例持久化。

以下是几个基本概念：

-   **Key-Value数据库**

    一种以键值对存储数据的一种数据库。Key是关键字，Value是值。

-   **非关系型数据库**

    区别于关系数据库，不保证遵循ACID（Atomic、Consistency、Isolation及Durability）特性，不采用关系模型来组织数据，数据之间无关系，扩展性好。

-   **偏好数据**

    用户经常访问和使用的数据。

**图 2**  首选项运行机制

![](figures/zh-cn_首选项运行机制.png)

## 目录

```
//foundation/distributeddatamgr/preferences
├── frameworks            # 框架层代码
│   └── js                # JS API的实现
│   │   └── napi          # napi代码实现
│   └── native            # 内部接口实现
├── interfaces            # 接口代码
│   └── inner_api         # 内部接口声明
└── test                  # 测试用例
    ├── js                # js用例
    └── native            # C++用例
```
## 约束

-   Key键为String类型，要求非空且长度不超过80个字符。

-   如果Value值为String类型，可以为空但是长度不超过8192个字符。

-   存储的数据量应该是轻量级的，建议存储的数据不超过一万条，否则会在内存方面产生较大的开销。

## 相关仓

- [分布式数据管理子系统](https://gitee.com/openharmony/docs/blob/master/zh-cn/readme/%E5%88%86%E5%B8%83%E5%BC%8F%E6%95%B0%E6%8D%AE%E7%AE%A1%E7%90%86%E5%AD%90%E7%B3%BB%E7%BB%9F.md)

- [**distributeddatamgr\_preferences**](https://gitee.com/openharmony/distributeddatamgr_preferences/blob/master/README_zh.md)