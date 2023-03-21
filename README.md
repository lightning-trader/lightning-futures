# lightning-trader

#### 介绍
轻量级期货量化开发框架，适合高频交易
QQ交流群:980550304
开发者QQ:137336521

#### 软件架构
1、lightning框架自下而上分3层架构(lightning.dll)
最底层是ctpapi以及高频测试用的高精度模拟器
中间层基于交易通道，封装了对订单的一系列处理逻辑，封装了共享内存使用逻辑，统一的事件机制，线程绑核，以及一个记录器
上层提供了C语言接口方便其他语言接入和集成
2、ltpp是lightning上层的c++封装(libltpp.lib)
libltpp主要封装了策略框架，策略的管理开发等操作，如果你对lightning源码没有兴趣，只希望应用它，那么可以直接使用编译好的lightning.dll基于libltpp去开发策略，


#### 安装教程

1、本项目目前采用cmake构建，只在win平台开发测试，如果需要在linux平台部署，需要自行编译测试
2、项目构建需要安装boost库，请自行安装
3、安装vs2022社区版，通过集成git工具克隆本项目，编译生成即可

#### 使用说明

1、doc文件夹下有编译好的二进制和配置文件实例以及2022年8月份螺纹钢测试数据，可用作参考
2、doc附带了demo策略2023年01月03日到2023年02月28日的实盘数据报告，可以用做参考

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
