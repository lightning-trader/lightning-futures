# lightning-trader

#### 介绍
lightning-trader轻量级期货量化开发库，适合高频交易，提供了通用的C语言接口供其他语言调用，以下是基于lightning-trader的多语言封装

- [lightning-strategy，提供基于lightning-trader的开源策略](https://gitee.com/pursue-wealth/lightning-strategy)

- QQ交流群:980550304
- 开发者QQ:137336521


#### 软件架构

lightning-trader框架自下而上分3层架构(lightning.dll)

- 最底层是ctpapi以及高频测试用的高精度模拟器
- 中间层基于交易通道，封装了对订单的一系列处理逻辑，封装了共享内存使用逻辑，统一的事件机制，线程绑核，以及一个记录器
- 上层提供了C语言接口方便其他语言接入和集成

![输入图片说明](doc/images/%E6%9E%B6%E6%9E%84%E5%9B%BE.png)

#### 线程模型

lightning-trader专为高频设计，采用3线程模型；

- 一个主线程控制程序流程；
- 一个低延时线程开启fast_mode以后会绑定的CPU，执行高频量化策略；
- 一个高延时线程负责做一些订单记录通知等功能

![输入图片说明](doc/images/%E7%BA%BF%E7%A8%8B%E6%A8%A1%E5%9E%8B.png)

#### 使用文档


1. 官方wiki：[点击这里](https://gitee.com/pursue-wealth/lightning-trader/wikis)
2. 知乎架构设计

    
- [Lightning Trader架构设计](https://zhuanlan.zhihu.com/p/622262304)
- [高频交易中如何处理低延时](https://zhuanlan.zhihu.com/p/622293141)
- [多线程程序设计的两种架构](https://zhuanlan.zhihu.com/p/622423099)


#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
