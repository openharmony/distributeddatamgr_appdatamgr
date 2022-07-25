# 轻量系统KV数据库（Lightweight KV store）

## 简介

**轻量系统KV数据库（Lightweight KV store）** 依托当前公共基础库提供的KV存储能力开发，为轻量系统设备应用提供键值对数据管理能力。在有进程的平台上，KV存储提供的参数管理，供单进程访问不能被其他进程使用。在此类平台上，KV存储作为基础库加载在应用进程，以保障不被其他进程访问。

1.  当前先支持轻量键值（KV）本地数据存储能力，后续会逐步支持其他更丰富的数据类型。
2.  轻量键值（KV）数据：数据有结构，文件轻量，具有简易事务性，单独提供一套专用的键值对接口

**目前，在轻量系统上默认关闭该特性，需要使用时请用户修改vendor_hisilicon仓配置以开启。**

## 目录

```
//foundation/distributeddatamgr/kv_store
├── frameworks            # 框架层代码
│   └── js                # JS API的实现
│   │   └── napi          # napi代码实现
│   └── native            # 内部接口实现
├── interfaces            # 接口代码
    └── inner_api         # 内部接口声明
```
## 约束

- KV大小及可存储条目数在平台可承受内可修改配置，轻量系统默认为小于Key(32byte)，Value(512byte)，通过修改编译宏修改；

- 依赖平台具有正常的文件创建、读写删除修改、锁等能力，针对不同平台（如LiteOS-M内核、LiteOS-A内核等）尽可能表现接口语义功能的不变；

- 由于平台能力差异数据库能力需要做相应裁剪，其中不同平台内部实现可能不同；

- 对于指定路径仅支持创建数据库单例，不支持同一路径创建多数据库实例对象。

## 相关仓

- [分布式数据管理子系统](https://gitee.com/openharmony/docs/blob/master/zh-cn/readme/%E5%88%86%E5%B8%83%E5%BC%8F%E6%95%B0%E6%8D%AE%E7%AE%A1%E7%90%86%E5%AD%90%E7%B3%BB%E7%BB%9F.md)

- [**distributeddatamgr\_kv_store**](https://gitee.com/openharmony/distributeddatamgr_kv_store/blob/master/README_zh.md)

- [third\_party\_sqlite](https://gitee.com/openharmony/third_party_sqlite)