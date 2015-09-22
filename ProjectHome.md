  1. 某公司有两个区：南区和东区。两区相距5km，如何在两区之间使用飞行器传递纸质文件？
  1. 为快递公司系统化地设计投递包裹业务(sent parcel)。
需要解决的主要问题有：
  * 目前四轴飞行器续航能力不足，一般20分钟左右，派送快件理想的单程时间也应该为2个小时。
  * 3G网络存在较大的时延，虽然通过该网络实时控制不能实现，但可以进行进行远程路径规划。


# 方案 #

---

![http://img2.ph.126.net/0Wzvggnm6IZqwwifJcv5sQ==/6597230295354822692.jpg](http://img2.ph.126.net/0Wzvggnm6IZqwwifJcv5sQ==/6597230295354822692.jpg)

![http://img8.ph.126.net/0dk6dw5D9t1wptQaKX6DnA==/6597233593889705358.jpg](http://img8.ph.126.net/0dk6dw5D9t1wptQaKX6DnA==/6597233593889705358.jpg)

![http://img9.ph.126.net/xuZVQYMPXk5A9e8mcKFq-A==/6597141234912974959.jpg](http://img9.ph.126.net/xuZVQYMPXk5A9e8mcKFq-A==/6597141234912974959.jpg)
### 单件派送流程 ###
  * 初始化：集散中心工作人员称重后将符合标准的快件在飞行器摄像头部进行条形码扫描，扫描完毕后联机查找用户信息，向用户发送通知短信，飞行器待机指示灯变绿。在工作人员将快件进行固定后，流水线将飞行器送至起落架。
  * 直线飞行：用户收到通知短信后，打开客户端输入密码进行确认，然后在百度地图界面指定具体位置。飞行器得到用户的确认收货信息后，飞行器从起落架起飞，以集散中心为原点建立坐标系，直线飞抵指定位置然后悬停，悬停后悬停指示灯变绿，向手机客户端发送悬停消息。
  * 实时画面：客户端收到悬停消息后进入实时画面，用户用手指在屏幕上拖动飞行器以指定满意位置，可多次调整。用户也可以使用屏幕上悬浮的方向键控制飞行器飞行。此工程中飞行器打开主灯发出蓝光。
  * 停靠：飞行器到达满意位置后，用户按主菜单键选择停靠，向飞行器发送停靠命令，飞行器安全停靠，用户从飞行器上获取自己的快件。用户取下快件后，快件指示灯由绿变红。如果电量不足，电量指示灯为红色或闪烁的红色，客户端提示用户充电，提示应包括消息和声音。
  * 返航：充电充满后，电量指示灯为绿色，客户端提示用户。用户将飞行器放在户外并在客户端主菜单里选择返航键。飞行器在以集散中心为原点的坐标系中直线返回，搜索起落架上的停靠点自主停靠。
Note:由于手机产品的多样性，客户端的性能并不统一，飞行器应当尽可能的自主飞行和停靠。当飞行器接近用户使用wifi进行连接后，飞行器应当穿过窗户并向强信号源中心靠拢，识别地面或桌面并自主停靠。

### 预算 ###
> 根据[市场调研](BoardCard.md)器件费用明细表如下：~~人工费每人20元/天~~
    * 飞行器相关共计<￥1200
|飞行器|名称|型号|价格|运费|数量|合计|重量|尺寸|
|:--|:-|:-|:-|:-|:-|:-|:-|:-|
|1  |四旋翼机架|玻纤x525| ￥125.0 | ￥12.0 |1 | ￥137.0 |  |  |
|2  |无刷直流电机|57BL52-230| ￥165.0| ￥- |4 | ￥660.0 |  |  |
|3  |无刷直流电调|AT-12A| ￥35.0 | ￥16.0 |4 | ￥152.0|  |  |
|4  |陀螺仪|ENC-03MB| ￥22.0| ￥12.0 |1 | ￥34.0 |  |  |

  * 板卡相关共计<￥800
|板卡|名称|型号|价格|运费|数量|合计|重量|尺寸（mm）|
|:-|:-|:-|:-|:-|:-|:-|:-|:-----|
|１ |3G(+GPS)|sim5218|￥390.0|￥15.0|1 |￥405.0|15g|58x26x4.5|
|２ |wifi|ST-MW-08S|￥92.0|￥22.0|1 |￥114.0|  |35.56x22x3.7|
|３ |摄像头|ov7670|￥15.0|￥12.0|1 |￥27.0|  |      |
|4 |单片机|arduino uno|60|13.5|1 |73.5|  |      |

### 开发工具 ###

[![](http://img0.ph.126.net/4aGfrn1fmdFy135fR9ipUw==/6597333649447701360.jpg)](http://fritzing.org/welcome/)

[![](http://img2.ph.126.net/OFfZk5zcuvBf5B8waDHv2g==/6597356739191887992.jpg)](http://www.android.com/)

[![](http://img0.ph.126.net/O5cjsrI7SaRFdH87Ye0Q-Q==/6597342445540723600.jpg)](http://www.arduino.cc/)

# 参考文献 #

---

  1. 宾夕法尼亚大学的[GRASP实验室](http://modiyanqing.blog.163.com/blog/static/1332284282012670450960/)的四轴小组可以实现飞行器穿过窗户并在斜坡上停靠。
  1. 谷歌公司的[Glass项目](http://modiyanqing.blog.163.com/blog/static/133228428201268104119236/)显示客户端的移动性是未来的趋势。
  1. Cooking Hacks [New 3G + GPS shield for Arduino](http://www.cooking-hacks.com/index.php/documentation/tutorials/arduino-3g-gprs-gsm-gps#step1)提供了一种移动视频传输的解决方法。
  1. 四轴飞行器的设计参考[Design Construction and Control of a Large Quadrotor Micro Air Vehicle](http://code.google.com/p/sentparcel/downloads/detail?name=Design%20Construction%20and%20Control%20of%20a%20Large%20Quadrotor%20Micro%20Air%20Vehicle.pdf&can=2&q=#makechanges)
  1. Arduino初步的系统学习参考[2010 Beginning Arduino](http://code.google.com/p/sentparcel/downloads/detail?name=2010%20Beginning%20Arduino.pdf&can=2&q=#makechanges)
  1. 手机客户端开发可以参考官方的训练教材[android training](http://developer.android.com/intl/zh-CN/training/index.html)
  1. 小型无人机四旋翼综述[微小型四旋翼无人机研究进展及关键技术浅析](http://sdrv.ms/Mc2acc)
  1. 参考已有的[四轴飞行器](Quadrotor.md)和任务需求来设计该飞行器的参数