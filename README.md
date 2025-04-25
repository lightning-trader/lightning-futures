# lightning-futures

#### 介绍
lightning-trader轻量级期货量化开发库，适合高频交易，3.5GHz处理器系统内部延时小于2微秒

- 本框架支持高频柜台接入，需要联系开发者
    - CTP2MINI 机房内从下单到收到委托回报时间300微秒，相比CTP13毫秒
    - 易盛V9.0、易盛V10启明星郑商所最优解决方案
    - 其他盛立，易达等高频柜台提供定制服务，具体联系QQ

- QQ技术群:689852151（LT19527）
- 开发者QQ:3844792568
- 捐助链接:[点击这里](https://gitee.com/lightning-trader/)

#### 软件架构

lightning-futures框架分两层架构

- 底层是交易系统API(ctp、ctpmini、易胜、盛立等)和高精度模拟器
- 上层分为高频交易框架，数据服务采集器，强化学习引擎
![输入图片说明](doc/images/%E6%9E%B6%E6%9E%84%E5%9B%BE.png)

#### 线程模型

lightning-futures专为高频设计，采用双线程模型；

- 一个主线程控制程序流程；
- 一个低延时线程开启fast_mode以后会绑定的CPU，执行高频量化策略；
![输入图片说明](doc/images/%E7%BA%BF%E7%A8%8B%E6%A8%A1%E5%9E%8B.png)

#### 使用文档


1. 官方wiki：[点击这里](https://gitee.com/lightning-trader/lightning-futures/wikis)
2. 知乎架构设计

    
- [Lightning Trader架构设计](https://zhuanlan.zhihu.com/p/622262304)
- [高频交易中如何处理低延时](https://zhuanlan.zhihu.com/p/622293141)
- [多线程程序设计的两种架构](https://zhuanlan.zhihu.com/p/622423099)
3. B站入门视频
- [LightningFutures量化交易开发（1）](https://www.bilibili.com/video/BV1TufUYMEqX/?share_source=copy_web&vd_source=12fb40b76f12e33f07bfc4a936d52013)
- [LightningFutures量化交易开发（2）](https://www.bilibili.com/video/BV1TufUYMEsP/?share_source=copy_web&vd_source=12fb40b76f12e33f07bfc4a936d52013)
- [LightningFutures量化交易开发（3）](https://www.bilibili.com/video/BV14ufUYME6T/?share_source=copy_web&vd_source=12fb40b76f12e33f07bfc4a936d52013)

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
